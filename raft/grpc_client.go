package raft

import (
	"bytes"
	"context"
	"time"

	"github.com/eraft-io/eraft/labgob"
	"github.com/eraft-io/eraft/raftpb"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials/insecure"
)

type RaftgRPCClient struct {
	client raftpb.RaftServiceClient
	conn   *grpc.ClientConn
}

func NewRaftgRPCClient(addr string) *RaftgRPCClient {
	conn, err := grpc.Dial(addr, grpc.WithTransportCredentials(insecure.NewCredentials()))
	if err != nil {
		panic(err)
	}
	client := raftpb.NewRaftServiceClient(conn)
	return &RaftgRPCClient{client: client, conn: conn}
}

func (c *RaftgRPCClient) RequestVote(args *RequestVoteRequest, reply *RequestVoteResponse) bool {
	ctx, cancel := context.WithTimeout(context.Background(), 100*time.Millisecond)
	defer cancel()
	req := &raftpb.RequestVoteRequest{
		Term:         int64(args.Term),
		CandidateId:  int64(args.CandidateId),
		LastLogIndex: int64(args.LastLogIndex),
		LastLogTerm:  int64(args.LastLogTerm),
	}
	resp, err := c.client.RequestVote(ctx, req)
	if err != nil {
		return false
	}
	reply.Term = int(resp.Term)
	reply.VoteGranted = resp.VoteGranted
	return true
}

func (c *RaftgRPCClient) AppendEntries(args *AppendEntriesRequest, reply *AppendEntriesResponse) bool {
	ctx, cancel := context.WithTimeout(context.Background(), 100*time.Millisecond)
	defer cancel()
	entries := make([]*raftpb.Entry, len(args.Entries))
	for i, e := range args.Entries {
		var command []byte
		if e.Command != nil {
			var ok bool
			command, ok = e.Command.([]byte)
			if !ok {
				// Fallback: if it's not []byte, try to encode it using labgob
				// This handles cases where old code might have stored structs in the log
				w := new(bytes.Buffer)
				enc := labgob.NewEncoder(w)
				if err := enc.Encode(e.Command); err == nil {
					command = w.Bytes()
				}
			}
		}
		entries[i] = &raftpb.Entry{
			Index:   int64(e.Index),
			Term:    int64(e.Term),
			Command: command,
		}
	}
	req := &raftpb.AppendEntriesRequest{
		Term:         int64(args.Term),
		LeaderId:     int64(args.LeaderId),
		PrevLogIndex: int64(args.PrevLogIndex),
		PrevLogTerm:  int64(args.PrevLogTerm),
		LeaderCommit: int64(args.LeaderCommit),
		Entries:      entries,
	}
	resp, err := c.client.AppendEntries(ctx, req)
	if err != nil {
		return false
	}
	reply.Term = int(resp.Term)
	reply.Success = resp.Success
	reply.ConflictIndex = int(resp.ConflictIndex)
	reply.ConflictTerm = int(resp.ConflictTerm)
	return true
}

func (c *RaftgRPCClient) InstallSnapshot(args *InstallSnapshotRequest, reply *InstallSnapshotResponse) bool {
	ctx, cancel := context.WithTimeout(context.Background(), 1*time.Second)
	defer cancel()
	req := &raftpb.InstallSnapshotRequest{
		Term:              int64(args.Term),
		LeaderId:          int64(args.LeaderId),
		LastIncludedIndex: int64(args.LastIncludedIndex),
		LastIncludedTerm:  int64(args.LastIncludedTerm),
		Data:              args.Data,
	}
	resp, err := c.client.InstallSnapshot(ctx, req)
	if err != nil {
		return false
	}
	reply.Term = int(resp.Term)
	return true
}
