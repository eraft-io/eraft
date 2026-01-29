package raft

import "fmt"

type RequestVoteRequest struct {
	Term         int
	CandidateId  int
	LastLogIndex int
	LastLogTerm  int
}

func (request RequestVoteRequest) String() string {
	return fmt.Sprintf("RequestVoteRequest{Term:%d, CandidatedId:%d, LastLogIndex:%d, LastLogTerm:%d}", request.Term, request.CandidateId, request.LastLogIndex, request.LastLogTerm)
}

type RequestVoteResponse struct {
	Term        int
	VoteGranted bool
}

func (response RequestVoteResponse) String() string {
	return fmt.Sprintf("RequestVoteResponse{Term:%d, VoteGranted:%t}", response.Term, response.VoteGranted)
}

type AppendEntriesRequest struct {
	Term         int
	LeaderId     int
	PrevLogIndex int
	PrevLogTerm  int
	LeaderCommit int
	Entries      []Entry
}

func (request AppendEntriesRequest) String() string {
	return fmt.Sprintf("AppendEntriesRequest{Term:%d, LeaderId:%d, PrevLogIndex:%d, PrevLogTerm:%d, LeaderCommit:%d, Entries:%v}", request.Term, request.LeaderId, request.PrevLogIndex, request.PrevLogTerm, request.LeaderCommit, request.Entries)
}

type AppendEntriesResponse struct {
	Term          int
	Success       bool
	ConflictIndex int
	ConflictTerm  int
}

func (response AppendEntriesResponse) String() string {
	return fmt.Sprintf("AppendEntriesResponse{Term:%d, Success:%t, ConflictIndex:%d, ConflictTerm:%d}", response.Term, response.Success, response.ConflictIndex, response.ConflictTerm)
}

type InstallSnapshotRequest struct {
	Term              int
	LeaderId          int
	LastIncludedIndex int
	LastIncludedTerm  int
	Data              []byte
}

func (request InstallSnapshotRequest) String() string {
	return fmt.Sprintf("InstallSnapshotRequest{Term:%d, LeaderId:%d, LastIncludedIndex:%d, LastIncludedTerm:%d, DataLen:%d}", request.Term, request.LeaderId, request.LastIncludedIndex, request.LastIncludedTerm, len(request.Data))
}

type InstallSnapshotResponse struct {
	Term int
}

func (response InstallSnapshotResponse) String() string {
	return fmt.Sprintf("InstallSnapshotResponse{Term:%d}", response.Term)
}
