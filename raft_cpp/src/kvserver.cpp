// kvserver.cpp — KVServer core implementation
// Corresponds to Go: kvraft/server.go

#include "raft/kvserver.h"


namespace raft {

// ── Factory ──────────────────────────────────────────────────

std::shared_ptr<KVServer> KVServer::Make(
    std::vector<std::shared_ptr<RaftPeer>> peers,
    int me,
    std::shared_ptr<Persister> persister,
    int maxRaftState,
    const std::string& dbPath)
{
    auto applyCh = std::make_shared<BlockingQueue<ApplyMsg>>();

    // Use shared_ptr + new (not make_shared) so enable_shared_from_this works
    std::shared_ptr<KVServer> kv(new KVServer());
    kv->maxRaftState_ = maxRaftState;
    kv->applyCh_      = applyCh;
    kv->rf_           = raft::Raft::Make(peers, me, persister, applyCh);
    kv->stateMachine_ = std::make_unique<RocksDBKV>(dbPath);

    // Restore state from snapshot
    auto snap = persister->ReadSnapshot();
    if (!snap.empty()) {
        kv->restoreSnapshot(snap);
    }

    // Start applier thread
    kv->applierThread_ = std::thread(&KVServer::applier, kv.get());

    DPrintf("KVServer %d has started\n", me);
    return kv;
}

KVServer::~KVServer() {
    Kill();
    if (applierThread_.joinable()) {
        applierThread_.join();
    }
    stateMachine_->Close();
}

// ── Command ──────────────────────────────────────────────────

void KVServer::HandleCommand(const CommandRequest& req, CommandResponse& resp) {
    DPrintf("Node %d processes CommandRequest %s\n", rf_->Me(), req.to_string().c_str());

    // Check for duplicate request (non-Get only)
    {
        std::lock_guard<std::mutex> lk(mu_);
        if (req.op != OpGet && isDuplicateRequest(req.clientId, req.commandId)) {
            auto it = lastOperations_.find(req.clientId);
            if (it != lastOperations_.end()) {
                resp = it->second.lastResponse;
                return;
            }
        }
    }

    // Serialize command and submit to Raft
    struct Command cmd;
    cmd.request = req;
    auto cmdBytes = kvser::serializeCommand(cmd);

    auto result = rf_->Start(cmdBytes);
    if (!result.isLeader) {
        resp.err = ErrWrongLeader;
        return;
    }

    // Wait for result via notify channel with timeout
    std::shared_ptr<BlockingQueue<CommandResponse>> ch;
    {
        std::lock_guard<std::mutex> lk(mu_);
        ch = getNotifyChan(result.index);
    }

    CommandResponse reply;
    bool got = ch->pop_for(reply, kExecuteTimeout);

    if (got) {
        resp = reply;
    } else {
        resp.err = ErrTimeout;
    }

    // Asynchronously clean up the notify channel
    std::thread([this, index = result.index]() {
        std::lock_guard<std::mutex> lk(mu_);
        removeOutdatedNotifyChan(index);
    }).detach();
}

// ── Kill ─────────────────────────────────────────────────────

void KVServer::Kill() {
    DPrintf("KVServer %d has been killed\n", rf_->Me());
    dead_.store(true);
    applyCh_->close();  // wake applier from pop()
    rf_->Kill();        // stop Raft threads
    // Note: stateMachine_->Close() is called in destructor after applier joins
}

// ── GetStatus ────────────────────────────────────────────────

KVServer::Status KVServer::GetStatus() {
    auto rs = rf_->GetStatus();
    return {rs.me, rs.state, rs.term, rs.lastApplied, rs.commitIndex,
            stateMachine_->Size() + rf_->GetRaftStateSize()};
}

// ── Applier ──────────────────────────────────────────────────

void KVServer::applier() {
    ApplyMsg message;
    while (applyCh_->pop(message)) {
        if (killed()) break;
        DPrintf("Node %d tries to apply message %s\n", rf_->Me(), message.to_string().c_str());

        // Process one message, then drain all pending messages before
        // checking snapshot.  This prevents the Raft log from growing
        // unbounded when the leader appends faster than we can apply.
        int lastIndex = -1;
        applyOneMessage(message, lastIndex);

        // Drain all remaining pending messages
        ApplyMsg next;
        while (!killed() && applyCh_->try_pop(next)) {
            applyOneMessage(next, lastIndex);
        }

        // Now check snapshot using the LAST applied index
        if (lastIndex >= 0) {
            std::lock_guard<std::mutex> lk(mu_);
            if (needSnapshot()) {
                takeSnapshot(lastIndex);
            }
        }
    }
}

// Apply a single ApplyMsg. Updates lastIndex to the most recent applied index.
void KVServer::applyOneMessage(const ApplyMsg& message, int& lastIndex) {
    DPrintf("Node %d applies message %s\n", rf_->Me(), message.to_string().c_str());

    if (message.command_valid) {
        std::lock_guard<std::mutex> lk(mu_);

        if (message.command_index <= lastApplied_) {
            DPrintf("Node %d discards outdated message %s\n",
                    rf_->Me(), message.to_string().c_str());
            return;
        }
        lastApplied_ = message.command_index;
        lastIndex = message.command_index;

        struct Command command = kvser::deserializeCommand(message.command);
        CommandResponse response;

        if (command.request.op != OpGet &&
            isDuplicateRequest(command.request.clientId, command.request.commandId))
        {
            DPrintf("Node %d skips duplicate command for client %lld\n",
                    rf_->Me(), (long long)command.request.clientId);
            response = lastOperations_[command.request.clientId].lastResponse;
        } else {
            response = applyLogToStateMachine(command);
            if (command.request.op != OpGet) {
                OperationContext ctx;
                ctx.maxAppliedCommandId = command.request.commandId;
                ctx.lastResponse = response;
                lastOperations_[command.request.clientId] = ctx;
            }
        }

        // Only notify if we are the leader in the current term
        auto [currentTerm, isLeader] = rf_->GetState();
        if (isLeader && message.command_term == currentTerm) {
            auto ch = getNotifyChan(message.command_index);
            ch->push(response);
        }
    } else if (message.snapshot_valid) {
        std::lock_guard<std::mutex> lk(mu_);
        if (rf_->CondInstallSnapshot(
                message.snapshot_term, message.snapshot_index, message.snapshot))
        {
            restoreSnapshot(message.snapshot);
            lastApplied_ = message.snapshot_index;
            lastIndex = message.snapshot_index;
        }
    }
}

// ── Snapshot helpers ─────────────────────────────────────────

bool KVServer::needSnapshot() {
    return maxRaftState_ != -1 && rf_->GetRaftStateSize() >= maxRaftState_;
}

void KVServer::takeSnapshot(int index) {
    // Dump KV data
    auto kvPairs = stateMachine_->DumpAll();

    // Serialize lastOperations
    std::vector<std::pair<int64_t, OperationContext>> ops(
        lastOperations_.begin(), lastOperations_.end());

    auto snapshot = kvser::serializeSnapshot(kvPairs, ops);
    rf_->Snapshot(index, snapshot);
}

void KVServer::restoreSnapshot(const std::vector<uint8_t>& snap) {
    if (snap.empty()) return;

    std::vector<std::pair<std::string, std::string>> kvPairs;
    std::vector<std::pair<int64_t, OperationContext>> ops;
    kvser::deserializeSnapshot(snap.data(), snap.size(), kvPairs, ops);

    // Restore KV state machine
    stateMachine_->BulkPut(kvPairs);

    // Restore lastOperations
    lastOperations_.clear();
    for (auto& [cid, ctx] : ops) {
        lastOperations_[cid] = ctx;
    }
}

// ── Duplicate detection ──────────────────────────────────────

bool KVServer::isDuplicateRequest(int64_t clientId, int64_t requestId) {
    auto it = lastOperations_.find(clientId);
    return it != lastOperations_.end() && requestId <= it->second.maxAppliedCommandId;
}

// ── Apply to state machine ───────────────────────────────────

CommandResponse KVServer::applyLogToStateMachine(const struct Command& cmd) {
    CommandResponse resp;
    switch (cmd.request.op) {
        case OpPut:
            resp.err = stateMachine_->Put(cmd.request.key, cmd.request.value);
            break;
        case OpAppend:
            resp.err = stateMachine_->Append(cmd.request.key, cmd.request.value);
            break;
        case OpGet:
            resp.err = stateMachine_->Get(cmd.request.key, resp.value);
            break;
    }
    return resp;
}

// ── Notify channels ──────────────────────────────────────────

std::shared_ptr<BlockingQueue<CommandResponse>> KVServer::getNotifyChan(int index) {
    auto it = notifyChans_.find(index);
    if (it == notifyChans_.end()) {
        auto ch = std::make_shared<BlockingQueue<CommandResponse>>();
        notifyChans_[index] = ch;
        return ch;
    }
    return it->second;
}

void KVServer::removeOutdatedNotifyChan(int index) {
    notifyChans_.erase(index);
}

} // namespace raft
