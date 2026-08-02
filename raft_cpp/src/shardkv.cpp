// shardkv.cpp — ShardKV core implementation
// Corresponds to Go: shardkv/server.go

#include "raft/shardkv.h"


namespace raft {

// ── Factory ──────────────────────────────────────────────────

std::shared_ptr<ShardKV> ShardKV::Make(
    std::vector<std::shared_ptr<RaftPeer>> peers,
    int me,
    std::shared_ptr<Persister> persister,
    int maxRaftState,
    int gid,
    std::shared_ptr<SCClerk> sc,
    ShardKVPeerProvider peerProvider)
{
    auto applyCh = std::make_shared<BlockingQueue<ApplyMsg>>();

    std::shared_ptr<ShardKV> kv(new ShardKV());
    kv->maxRaftState_  = maxRaftState;
    kv->applyCh_       = applyCh;
    kv->rf_            = raft::Raft::Make(peers, me, persister, applyCh);
    kv->gid_           = gid;
    kv->sc_            = sc;
    kv->peerProvider_  = std::move(peerProvider);
    kv->currentConfig_ = DefaultSCConfig();
    kv->lastConfig_    = DefaultSCConfig();

    // Initialize shard status
    kv->initStateMachines();

    // Restore from snapshot if available
    auto snap = persister->ReadSnapshot();
    if (!snap.empty()) {
        kv->restoreSnapshot(snap);
    }

    // Use weak_ptr in lambdas to avoid circular references (threads hold shared_ptr)
    std::weak_ptr<ShardKV> weakKv = kv;

    // Start all threads
    kv->applierThread_     = std::thread(&ShardKV::applier, kv.get());
    kv->configureThread_   = std::thread(&ShardKV::monitorLoop, kv.get(),
        [weakKv]() { auto kv = weakKv.lock(); if (kv) kv->configureAction(); }, kConfigureMonitorTimeout);
    kv->migrationThread_   = std::thread(&ShardKV::monitorLoop, kv.get(),
        [weakKv]() { auto kv = weakKv.lock(); if (kv) kv->migrationAction(); }, kMigrationMonitorTimeout);
    kv->gcThread_          = std::thread(&ShardKV::monitorLoop, kv.get(),
        [weakKv]() { auto kv = weakKv.lock(); if (kv) kv->gcAction(); }, kGCMonitorTimeout);
    kv->emptyEntryThread_  = std::thread(&ShardKV::monitorLoop, kv.get(),
        [weakKv]() { auto kv = weakKv.lock(); if (kv) kv->checkEntryInCurrentTermAction(); }, kEmptyEntryDetectorTimeout);

    DPrintf("ShardKV %d (group %d) has started\n", me, gid);
    return kv;
}

ShardKV::~ShardKV() {
    Kill();
    // Destroy Raft first to stop its threads before joining ours
    rf_.reset();
    // Join threads; detach on failure to avoid std::terminate
    auto safeJoin = [](std::thread& t) {
        try { if (t.joinable()) t.join(); } catch (...) { t.detach(); }
    };
    safeJoin(applierThread_);
    safeJoin(configureThread_);
    safeJoin(migrationThread_);
    safeJoin(gcThread_);
    safeJoin(emptyEntryThread_);
}

// ── Command (KV operation entry point) ───────────────────────

void ShardKV::Command(const SKVCommandRequest& req, SKVCommandResponse& resp) {
    DPrintf("Node %d{Group %d} processes SKVCommandRequest %s\n",
            rf_->Me(), gid_, req.to_string().c_str());

    int shardID = key2shard(req.key);
    {
        std::lock_guard<std::mutex> lk(mu_);
        // Return result directly if duplicated (non-Get)
        if (req.op != OpGet && isDuplicateRequest(req.clientId, req.commandId)) {
            auto it = lastOperations_.find(req.clientId);
            if (it != lastOperations_.end()) {
                resp = it->second.lastResponse;
                return;
            }
        }
        // Return ErrWrongGroup if can't serve this key
        if (!canServe(shardID)) {
            resp.err = SKV_ErrWrongGroup;
            return;
        }
    }

    // Build Operation command and execute through Raft
    auto cmd = skvser::makeOperationCmd(req);
    Execute(cmd, resp);
}

// ── Execute (submit to Raft and wait) ────────────────────────

void ShardKV::Execute(const SKVCommand& cmd, SKVCommandResponse& resp) {
    auto cmdBytes = skvser::serializeCommand(cmd);

    auto result = rf_->Start(cmdBytes);
    if (!result.isLeader) {
        resp.err = SKV_ErrWrongLeader;
        return;
    }

    // Wait for result via notify channel with timeout
    std::shared_ptr<BlockingQueue<SKVCommandResponse>> ch;
    {
        std::lock_guard<std::mutex> lk(mu_);
        ch = getNotifyChan(result.index);
    }

    SKVCommandResponse reply;
    bool got = ch->pop_for(reply, kSKVExecuteTimeout);

    if (got) {
        resp = reply;
    } else {
        resp.err = SKV_ErrTimeout;
    }

    // Asynchronously clean up the notify channel
    std::weak_ptr<ShardKV> weakSelf = shared_from_this();
    std::thread([weakSelf, index = result.index]() {
        auto self = weakSelf.lock();
        if (!self) return;
        std::lock_guard<std::mutex> lk(self->mu_);
        self->removeOutdatedNotifyChan(index);
    }).detach();
}

// ── GetShardsData (cross-group pull) ─────────────────────────

void ShardKV::GetShardsData(const ShardOperationRequest& req, ShardOperationResponse& resp) {
    // Only leader responds
    auto [term, isLeader] = rf_->GetState();
    (void)term;
    if (!isLeader) {
        resp.err = SKV_ErrWrongLeader;
        return;
    }

    std::lock_guard<std::mutex> lk(mu_);

    if (currentConfig_.Num < req.configNum) {
        resp.err = SKV_ErrNotReady;
        return;
    }

    resp.shards.clear();
    for (int shardID : req.shardIDs) {
        resp.shards[shardID] = shards_[shardID];
    }

    resp.lastOperations.clear();
    for (auto& [cid, ctx] : lastOperations_) {
        resp.lastOperations[cid] = ctx;
    }

    resp.configNum = req.configNum;
    resp.err = SKV_OK;
}

// ── DeleteShardsData (cross-group GC) ────────────────────────

void ShardKV::DeleteShardsData(const ShardOperationRequest& req, ShardOperationResponse& resp) {
    auto [term, isLeader] = rf_->GetState();
    (void)term;
    if (!isLeader) {
        resp.err = SKV_ErrWrongLeader;
        return;
    }

    {
        std::lock_guard<std::mutex> lk(mu_);
        if (currentConfig_.Num > req.configNum) {
            // Already past this config, treat as success
            resp.err = SKV_OK;
            return;
        }
    }

    // Submit delete through Raft
    auto cmd = skvser::makeDeleteShardsCmd(req);
    SKVCommandResponse cmdResp;
    Execute(cmd, cmdResp);
    resp.err = cmdResp.err;
}

// ── Diagnostics ──────────────────────────────────────────────

ShardKV::DebugInfo ShardKV::GetDebugInfo() {
    std::lock_guard<std::mutex> lk(mu_);
    DebugInfo info;
    info.configNum = currentConfig_.Num;
    info.nonServingCount = 0;
    for (int i = 0; i < NShards; ++i) {
        info.shardStatuses[i] = shardStatus_[i];
        if (shardStatus_[i] != Serving) {
            info.nonServingCount++;
        }
    }
    return info;
}

// ── Kill ─────────────────────────────────────────────────────

void ShardKV::Kill() {
    DPrintf("ShardKV %d{Group %d} has been killed\n", rf_->Me(), gid_);
    dead_.store(true);
    applyCh_->close();
    rf_->Kill();
}

// ── Applier ──────────────────────────────────────────────────

void ShardKV::applier() {
    ApplyMsg message;
    while (applyCh_->pop(message)) {
        if (killed()) break;
        DPrintf("Node %d{Group %d} tries to apply message %s\n",
                rf_->Me(), gid_, message.to_string().c_str());

        if (message.command_valid) {
            std::lock_guard<std::mutex> lk(mu_);

            if (message.command_index <= lastApplied_) {
                continue;
            }
            lastApplied_ = message.command_index;

            auto command = skvser::deserializeCommand(message.command);
            SKVCommandResponse response;

            switch (command.type) {
                case SKV_Operation: {
                    const uint8_t* p = command.data.data();
                    auto req = skvser::deserializeRequest(p);
                    response = applyOperation(req);
                    break;
                }
                case SKV_Configuration: {
                    const uint8_t* p = command.data.data();
                    auto config = skvser::deserializeConfig(p);
                    response = applyConfiguration(config);
                    break;
                }
                case SKV_InsertShards: {
                    const uint8_t* p = command.data.data();
                    auto info = skvser::deserializeShardOpResp(p);
                    response = applyInsertShards(info);
                    break;
                }
                case SKV_DeleteShards: {
                    const uint8_t* p = command.data.data();
                    auto req = skvser::deserializeShardOpReq(p);
                    response = applyDeleteShards(req);
                    break;
                }
                case SKV_EmptyEntry:
                    response.err = SKV_OK;
                    break;
            }

            // Notify waiting Execute call if term matches
            auto [currentTerm, isLeader] = rf_->GetState();
            (void)isLeader;
            if (message.command_term == currentTerm) {
                auto ch = getNotifyChan(message.command_index);
                ch->push(response);
            }

            // Check snapshot
            if (needSnapshot()) {
                takeSnapshot(message.command_index);
            }

        } else if (message.snapshot_valid) {
            std::lock_guard<std::mutex> lk(mu_);
            if (rf_->CondInstallSnapshot(
                    message.snapshot_term, message.snapshot_index, message.snapshot)) {
                restoreSnapshot(message.snapshot);
                lastApplied_ = message.snapshot_index;
            }
        }
    }
}

// ── Apply handlers ───────────────────────────────────────────

SKVCommandResponse ShardKV::applyOperation(const SKVCommandRequest& req) {
    int shardID = key2shard(req.key);

    if (!canServe(shardID)) {
        return {SKV_ErrWrongGroup, ""};
    }

    // Duplicate check
    if (req.op != OpGet && isDuplicateRequest(req.clientId, req.commandId)) {
        auto it = lastOperations_.find(req.clientId);
        if (it != lastOperations_.end()) {
            return it->second.lastResponse;
        }
    }

    SKVCommandResponse resp;
    switch (req.op) {
        case OpPut:
            shards_[shardID][req.key] = req.value;
            resp.err = SKV_OK;
            break;
        case OpAppend: {
            auto& val = shards_[shardID][req.key];
            val += req.value;
            resp.value = val;
            resp.err = SKV_OK;
            break;
        }
        case OpGet: {
            auto it = shards_[shardID].find(req.key);
            if (it != shards_[shardID].end()) {
                resp.value = it->second;
                resp.err = SKV_OK;
            } else {
                resp.err = SKV_ErrNoKey;
            }
            break;
        }
    }

    // Update duplicate detection (non-Get only)
    if (req.op != OpGet) {
        SKVOperationContext ctx;
        ctx.maxAppliedCommandId = req.commandId;
        ctx.lastResponse = resp;
        lastOperations_[req.clientId] = ctx;
    }

    return resp;
}

SKVCommandResponse ShardKV::applyConfiguration(const SCConfig& nextConfig) {
    if (nextConfig.Num == currentConfig_.Num + 1) {
        DPrintf("Node %d{Group %d} updates config from %d to %d\n",
                rf_->Me(), gid_, currentConfig_.Num, nextConfig.Num);
        updateShardStatus(nextConfig);
        lastConfig_ = currentConfig_;
        currentConfig_ = nextConfig;
        return {SKV_OK, ""};
    }
    return {SKV_ErrOutDated, ""};
}

SKVCommandResponse ShardKV::applyInsertShards(const ShardOperationResponse& info) {
    if (info.configNum == currentConfig_.Num) {
        for (auto& [shardId, shardData] : info.shards) {
            if (shardStatus_[shardId] == Pulling) {
                for (auto& [k, v] : shardData) {
                    shards_[shardId][k] = v;
                }
                shardStatus_[shardId] = GCing;
            }
            // If not Pulling, skip (may be duplicate or already processed)
        }
        // Merge last operations
        for (auto& [cid, ctx] : info.lastOperations) {
            auto it = lastOperations_.find(cid);
            if (it == lastOperations_.end() ||
                it->second.maxAppliedCommandId < ctx.maxAppliedCommandId) {
                lastOperations_[cid] = ctx;
            }
        }
        return {SKV_OK, ""};
    }
    return {SKV_ErrOutDated, ""};
}

SKVCommandResponse ShardKV::applyDeleteShards(const ShardOperationRequest& req) {
    if (req.configNum == currentConfig_.Num) {
        for (int shardId : req.shardIDs) {
            if (shardStatus_[shardId] == GCing) {
                shardStatus_[shardId] = Serving;
            } else if (shardStatus_[shardId] == BePulling) {
                shardStatus_[shardId] = Serving;
                clearShardData(shardId);
            }
        }
        return {SKV_OK, ""};
    }
    return {SKV_OK, ""};  // already past, treat as success
}

// ── Monitor actions ──────────────────────────────────────────

void ShardKV::monitorLoop(std::function<void()> action,
                           std::chrono::milliseconds timeout) {
    while (!killed()) {
        auto [term, isLeader] = rf_->GetState();
        (void)term;
        if (isLeader) {
            action();
        }
        // Sleep in small increments for responsiveness to Kill()
        auto remaining = timeout;
        while (!killed() && remaining.count() > 0) {
            auto sleepTime = std::min(remaining, std::chrono::milliseconds(50));
            std::this_thread::sleep_for(sleepTime);
            remaining -= sleepTime;
        }
    }
}

void ShardKV::configureAction() {
    bool canPerformNextConfig = true;
    int currentConfigNum;
    {
        std::lock_guard<std::mutex> lk(mu_);
        for (int i = 0; i < NShards; ++i) {
            if (shardStatus_[i] != Serving) {
                canPerformNextConfig = false;
                break;
            }
        }
        currentConfigNum = currentConfig_.Num;
    }

    if (canPerformNextConfig) {
        auto nextConfig = sc_->Query(currentConfigNum + 1);
        if (nextConfig.Num == currentConfigNum + 1) {
            auto cmd = skvser::makeConfigurationCmd(nextConfig);
            SKVCommandResponse resp;
            Execute(cmd, resp);
        }
    }
}

void ShardKV::migrationAction() {
    std::map<int, std::vector<int>> gid2shardIDs;
    int configNum;
    {
        std::lock_guard<std::mutex> lk(mu_);
        gid2shardIDs = getShardIDsByStatus(Pulling);
        configNum = currentConfig_.Num;
    }

    for (auto& [gid, shardIDs] : gid2shardIDs) {
        ShardOperationRequest pullReq;
        pullReq.configNum = configNum;
        pullReq.shardIDs  = shardIDs;

        // Try each server in the source group
        static const std::vector<std::string> empty_servers;
        const auto& servers = lastConfig_.Groups.count(gid) ? lastConfig_.Groups.at(gid) :
                        (currentConfig_.Groups.count(gid) ? currentConfig_.Groups.at(gid) :
                         empty_servers);

        bool success = false;
        for (size_t i = 0; i < servers.size() && !success; ++i) {
            if (!peerProvider_) continue;
            auto peer = peerProvider_(gid, static_cast<int>(i), servers[i]);
            if (!peer) continue;

            ShardOperationResponse resp;
            if (peer->GetShardsData(pullReq, resp) && resp.err == SKV_OK) {
                // Submit InsertShards through Raft
                auto cmd = skvser::makeInsertShardsCmd(resp);
                SKVCommandResponse cmdResp;
                Execute(cmd, cmdResp);
                success = true;
            }
        }
    }
}

void ShardKV::gcAction() {
    std::map<int, std::vector<int>> gid2shardIDs;
    int configNum;
    {
        std::lock_guard<std::mutex> lk(mu_);
        gid2shardIDs = getShardIDsByStatus(GCing);
        configNum = currentConfig_.Num;
    }

    for (auto& [gid, shardIDs] : gid2shardIDs) {
        ShardOperationRequest gcReq;
        gcReq.configNum = configNum;
        gcReq.shardIDs  = shardIDs;

        static const std::vector<std::string> empty_servers_gc;
        const auto& servers = lastConfig_.Groups.count(gid) ? lastConfig_.Groups.at(gid) :
                        (currentConfig_.Groups.count(gid) ? currentConfig_.Groups.at(gid) :
                         empty_servers_gc);

        bool success = false;
        for (size_t i = 0; i < servers.size() && !success; ++i) {
            if (!peerProvider_) continue;
            auto peer = peerProvider_(gid, static_cast<int>(i), servers[i]);
            if (!peer) continue;

            ShardOperationResponse resp;
            if (peer->DeleteShardsData(gcReq, resp) && resp.err == SKV_OK) {
                // Submit local DeleteShards through Raft
                auto cmd = skvser::makeDeleteShardsCmd(gcReq);
                SKVCommandResponse cmdResp;
                Execute(cmd, cmdResp);
                success = true;
            }
        }
    }
}

void ShardKV::checkEntryInCurrentTermAction() {
    if (!rf_->HasLogInCurrentTerm()) {
        auto cmd = skvser::makeEmptyEntryCmd();
        SKVCommandResponse resp;
        Execute(cmd, resp);
    }
}

// ── Helpers ──────────────────────────────────────────────────

bool ShardKV::canServe(int shardID) {
    return currentConfig_.Shards[shardID] == gid_ &&
           (shardStatus_[shardID] == Serving || shardStatus_[shardID] == GCing);
}

bool ShardKV::isDuplicateRequest(int64_t clientId, int64_t requestId) {
    auto it = lastOperations_.find(clientId);
    return it != lastOperations_.end() && requestId <= it->second.maxAppliedCommandId;
}

void ShardKV::updateShardStatus(const SCConfig& nextConfig) {
    for (int i = 0; i < NShards; ++i) {
        int prevOwner = currentConfig_.Shards[i];
        int nextOwner = nextConfig.Shards[i];

        if (prevOwner != gid_ && nextOwner == gid_) {
            // Gaining ownership of this shard
            if (prevOwner == 0) {
                shardStatus_[i] = Serving;
            } else {
                shardStatus_[i] = Pulling;
            }
        } else if (prevOwner == gid_ && nextOwner != gid_) {
            // Losing ownership of this shard
            if (nextOwner == 0) {
                // Going to gid 0
            } else {
                shardStatus_[i] = BePulling;
            }
        } else if (prevOwner == gid_ && nextOwner == gid_) {
            // Retaining ownership
            if (shardStatus_[i] == Pulling) {
                // Keep Pulling
            } else {
                shardStatus_[i] = Serving;
            }
        }
    }
}

std::map<int, std::vector<int>> ShardKV::getShardIDsByStatus(ShardStatus status) {
    std::map<int, std::vector<int>> gid2shardIDs;
    for (int i = 0; i < NShards; ++i) {
        if (shardStatus_[i] == status) {
            int gid = lastConfig_.Shards[i];
            if (gid != 0) {
                gid2shardIDs[gid].push_back(i);
            }
        }
    }
    return gid2shardIDs;
}

void ShardKV::clearShardData(int shardID) {
    shards_[shardID].clear();
}

// ── Snapshot ─────────────────────────────────────────────────

bool ShardKV::needSnapshot() {
    return maxRaftState_ != -1 && rf_->GetRaftStateSize() >= maxRaftState_;
}

void ShardKV::takeSnapshot(int index) {
    // Convert unordered_map to ordered map for snapshot serialization
    std::map<int64_t, SKVOperationContext> orderedOps(
        lastOperations_.begin(), lastOperations_.end());
    auto snapshot = skvser::serializeSnapshot(
        shards_, shardStatus_, orderedOps, currentConfig_, lastConfig_);
    rf_->Snapshot(index, snapshot);
}

void ShardKV::restoreSnapshot(const std::vector<uint8_t>& snap) {
    if (snap.empty()) {
        initStateMachines();
        return;
    }
    std::map<int64_t, SKVOperationContext> orderedOps;
    skvser::deserializeSnapshot(
        snap.data(), snap.size(),
        shards_, shardStatus_, orderedOps, currentConfig_, lastConfig_);
    lastOperations_.clear();
    for (auto& [cid, ctx] : orderedOps) {
        lastOperations_[cid] = ctx;
    }
}

void ShardKV::initStateMachines() {
    for (int i = 0; i < NShards; ++i) {
        shardStatus_[i] = Serving;
        shards_[i].clear();
    }
}

// ── Notify channels ──────────────────────────────────────────

std::shared_ptr<BlockingQueue<SKVCommandResponse>> ShardKV::getNotifyChan(int index) {
    auto it = notifyChans_.find(index);
    if (it == notifyChans_.end()) {
        auto ch = std::make_shared<BlockingQueue<SKVCommandResponse>>();
        notifyChans_[index] = ch;
        return ch;
    }
    return it->second;
}

void ShardKV::removeOutdatedNotifyChan(int index) {
    notifyChans_.erase(index);
}

} // namespace raft
