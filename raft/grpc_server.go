package raft

import (
	"context"

	"github.com/eraft-io/eraft/raftpb"
)

type RaftgRPCServer struct {
	raftpb.UnimplementedRaftServiceServer
	rf *Raft
}

func NewRaftgRPCServer(rf *Raft) *RaftgRPCServer {
	return &RaftgRPCServer{rf: rf}
}

func (s *RaftgRPCServer) RequestVote(ctx context.Context, req *raftpb.RequestVoteRequest) (*raftpb.RequestVoteResponse, error) {
	args := &RequestVoteRequest{
		Term:         int(req.Term),
		CandidateId:  int(req.CandidateId),
		LastLogIndex: int(req.LastLogIndex),
		LastLogTerm:  int(req.LastLogTerm),
	}
	reply := &RequestVoteResponse{}
	s.rf.RequestVote(args, reply)
	return &raftpb.RequestVoteResponse{
		Term:        int64(reply.Term),
		VoteGranted: reply.VoteGranted,
	}, nil
}

func (s *RaftgRPCServer) AppendEntries(ctx context.Context, req *raftpb.AppendEntriesRequest) (*raftpb.AppendEntriesResponse, error) {
	entries := make([]Entry, len(req.Entries))
	for i, e := range req.Entries {
		entries[i] = Entry{
			Index:   int(e.Index),
			Term:    int(e.Term),
			Command: e.Command,
		}
	}
	args := &AppendEntriesRequest{
		Term:         int(req.Term),
		LeaderId:     int(req.LeaderId),
		PrevLogIndex: int(req.PrevLogIndex),
		PrevLogTerm:  int(req.PrevLogTerm),
		LeaderCommit: int(req.LeaderCommit),
		Entries:      entries,
	}
	reply := &AppendEntriesResponse{}
	s.rf.AppendEntries(args, reply)
	return &raftpb.AppendEntriesResponse{
		Term:          int64(reply.Term),
		Success:       reply.Success,
		ConflictIndex: int64(reply.ConflictIndex),
		ConflictTerm:  int64(reply.ConflictTerm),
	}, nil
}

func (s *RaftgRPCServer) InstallSnapshot(ctx context.Context, req *raftpb.InstallSnapshotRequest) (*raftpb.InstallSnapshotResponse, error) {
	args := &InstallSnapshotRequest{
		Term:              int(req.Term),
		LeaderId:          int(req.LeaderId),
		LastIncludedIndex: int(req.LastIncludedIndex),
		LastIncludedTerm:  int(req.LastIncludedTerm),
		Data:              req.Data,
	}
	reply := &InstallSnapshotResponse{}
	s.rf.InstallSnapshot(args, reply)
	return &raftpb.InstallSnapshotResponse{
		Term: int64(reply.Term),
	}, nil
}
