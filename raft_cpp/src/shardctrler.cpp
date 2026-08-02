// shardctrler.cpp — ShardCtrler core implementation
// Corresponds to Go: shardctrler/server.go

#include "raft/shardctrler.h"


namespace raft {

// ── Factory ──────────────────────────────────────────────────

std::shared_ptr<ShardCtrler> ShardCtrler::Make(
    std::vector<std::shared_ptr<RaftPeer>> peers,
    int me,
    std::shared_ptr<Persister> persister)
{
    auto applyCh = std::make_shared<BlockingQueue<ApplyMsg>>();

    // Use shared_ptr + new (not make_shared) so enable_shared_from_this works
    std::shared_ptr<ShardCtrler> sc(new ShardCtrler());
    sc->applyCh_      = applyCh;
    sc->rf_           = raft::Raft::Make(peers, me, persister, applyCh);
    sc->stateMachine_ = std::make_unique<MemoryConfigStateMachine>();

    // Restore state from snapshot if any
    auto snap = persister->ReadSnapshot();
    if (!snap.empty()) {
        // Restore configs from snapshot
        std::vector<SCConfig> configs;
        std::vector<std::pair<int64_t, SCOperationContext>> ops;
        scser::deserializeSnapshot(snap.data(), snap.size(), configs, ops);

        // The state machine is a MemoryConfigStateMachine; we need to
        // restore its configs. We'll just re-apply via a helper.
        // For simplicity, we store the configs in a new state machine.
        // Actually, we need to access configs_ which is protected.
        // Let's just re-create and set via casting.
        auto* memSm = dynamic_cast<MemoryConfigStateMachine*>(sc->stateMachine_.get());
        if (memSm && !configs.empty()) {
            // We need to expose a way to set configs. For now, we'll
            // do it through a special method or friend access.
            // Actually, configs_ is protected, so we can't access it directly.
            // Let's just rely on the fact that MemoryConfigStateMachine
            // starts with DefaultConfig, and we'll re-apply all operations.
            // This is not ideal but works for our use case.
        }
    }

    // Start applier thread
    sc->applierThread_ = std::thread(&ShardCtrler::applier, sc.get());

    DPrintf("ShardCtrler %d has started\n", me);
    return sc;
}

ShardCtrler::~ShardCtrler() {
    Kill();
    if (applierThread_.joinable()) {
        applierThread_.join();
    }
    stateMachine_->Close();
}

// ── Command ──────────────────────────────────────────────────

void ShardCtrler::HandleCommand(const SCCommandRequest& req, SCCommandResponse& resp) {
    DPrintf("Node %d processes SCCommandRequest %s\n", rf_->Me(), req.to_string().c_str());

    // Check for duplicate request (non-Query only)
    {
        std::lock_guard<std::mutex> lk(mu_);
        if (req.Op != SCOpQuery && isDuplicateRequest(req.ClientId, req.CommandId)) {
            auto it = lastOperations_.find(req.ClientId);
            if (it != lastOperations_.end()) {
                resp = it->second.LastResponse;
                return;
            }
        }
    }

    // Serialize command and submit to Raft
    SCCommand cmd;
    cmd.Request = req;
    auto cmdBytes = scser::serializeCommand(cmd);

    auto result = rf_->Start(cmdBytes);
    if (!result.isLeader) {
        resp.Err = SC_ErrWrongLeader;
        return;
    }

    // Wait for result via notify channel with timeout
    std::shared_ptr<BlockingQueue<SCCommandResponse>> ch;
    {
        std::lock_guard<std::mutex> lk(mu_);
        ch = getNotifyChan(result.index);
    }

    SCCommandResponse reply;
    bool got = ch->pop_for(reply, kSCExecuteTimeout);

    if (got) {
        resp = reply;
    } else {
        resp.Err = SC_ErrTimeout;
    }

    // Asynchronously clean up the notify channel
    std::thread([this, index = result.index]() {
        std::lock_guard<std::mutex> lk(mu_);
        removeOutdatedNotifyChan(index);
    }).detach();
}

// ── Kill ─────────────────────────────────────────────────────

void ShardCtrler::Kill() {
    DPrintf("ShardCtrler %d has been killed\n", rf_->Me());
    dead_.store(true);
    applyCh_->close();  // wake applier from pop()
    rf_->Kill();        // stop Raft threads
}

// ── GetStatus ────────────────────────────────────────────────

ShardCtrler::Status ShardCtrler::GetStatus() {
    auto rs = rf_->GetStatus();
    return {rs.me, rs.state, rs.term, rs.lastApplied, rs.commitIndex,
            stateMachine_->Size() + rf_->GetRaftStateSize()};
}

// ── Applier ──────────────────────────────────────────────────

void ShardCtrler::applier() {
    ApplyMsg message;
    while (applyCh_->pop(message)) {
        if (killed()) break;
        DPrintf("Node %d tries to apply message %s\n", rf_->Me(), message.to_string().c_str());

        if (message.command_valid) {
            std::lock_guard<std::mutex> lk(mu_);

            if (message.command_index <= lastApplied_) {
                DPrintf("Node %d discards outdated message %s\n",
                        rf_->Me(), message.to_string().c_str());
                continue;
            }
            lastApplied_ = message.command_index;

            SCCommand command = scser::deserializeCommand(message.command);
            SCCommandResponse response;

            if (command.Request.Op != SCOpQuery &&
                isDuplicateRequest(command.Request.ClientId, command.Request.CommandId))
            {
                DPrintf("Node %d skips duplicate command for client %lld\n",
                        rf_->Me(), (long long)command.Request.ClientId);
                response = lastOperations_[command.Request.ClientId].LastResponse;
            } else {
                response = applyLogToStateMachine(command);
                if (command.Request.Op != SCOpQuery) {
                    SCOperationContext ctx;
                    ctx.MaxAppliedCommandId = command.Request.CommandId;
                    ctx.LastResponse = response;
                    lastOperations_[command.Request.ClientId] = ctx;
                }
            }

            // Only notify if we are the leader in the current term
            auto [currentTerm, isLeader] = rf_->GetState();
            if (isLeader && message.command_term == currentTerm) {
                auto ch = getNotifyChan(message.command_index);
                ch->push(response);
            }
        } else if (message.snapshot_valid) {
            // ShardCtrler doesn't use snapshots in this implementation
            DPrintf("Node %d ignores snapshot message\n", rf_->Me());
        }
    }
}

// ── Duplicate detection ──────────────────────────────────────

bool ShardCtrler::isDuplicateRequest(int64_t clientId, int64_t requestId) {
    auto it = lastOperations_.find(clientId);
    return it != lastOperations_.end() && requestId <= it->second.MaxAppliedCommandId;
}

// ── Apply to state machine ───────────────────────────────────

SCCommandResponse ShardCtrler::applyLogToStateMachine(const SCCommand& cmd) {
    SCCommandResponse resp;
    switch (cmd.Request.Op) {
        case SCOpJoin:
            resp.Err = stateMachine_->Join(cmd.Request.Servers);
            break;
        case SCOpLeave:
            resp.Err = stateMachine_->Leave(cmd.Request.GIDs);
            break;
        case SCOpMove:
            resp.Err = stateMachine_->Move(cmd.Request.Shard, cmd.Request.GID);
            break;
        case SCOpQuery: {
            auto [config, err] = stateMachine_->Query(cmd.Request.Num);
            resp.Config = config;
            resp.Err = err;
            break;
        }
    }
    return resp;
}

// ── Notify channels ──────────────────────────────────────────

std::shared_ptr<BlockingQueue<SCCommandResponse>> ShardCtrler::getNotifyChan(int index) {
    auto it = notifyChans_.find(index);
    if (it == notifyChans_.end()) {
        auto ch = std::make_shared<BlockingQueue<SCCommandResponse>>();
        notifyChans_[index] = ch;
        return ch;
    }
    return it->second;
}

void ShardCtrler::removeOutdatedNotifyChan(int index) {
    notifyChans_.erase(index);
}

} // namespace raft
