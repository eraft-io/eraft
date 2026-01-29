package raft

import "github.com/eraft-io/eraft/labrpc"

type LabrpcPeer struct {
	End *labrpc.ClientEnd
}

func (p *LabrpcPeer) RequestVote(args *RequestVoteRequest, reply *RequestVoteResponse) bool {
	return p.End.Call("Raft.RequestVote", args, reply)
}

func (p *LabrpcPeer) AppendEntries(args *AppendEntriesRequest, reply *AppendEntriesResponse) bool {
	return p.End.Call("Raft.AppendEntries", args, reply)
}

func (p *LabrpcPeer) InstallSnapshot(args *InstallSnapshotRequest, reply *InstallSnapshotResponse) bool {
	return p.End.Call("Raft.InstallSnapshot", args, reply)
}

func CastLabrpcToRaftPeers(peers []*labrpc.ClientEnd) []RaftPeer {
	raftPeers := make([]RaftPeer, len(peers))
	for i, p := range peers {
		raftPeers[i] = &LabrpcPeer{End: p}
	}
	return raftPeers
}

func CastLabrpcToNames(peers []*labrpc.ClientEnd) []string {
	names := make([]string, len(peers))
	for i := range peers {
		// This is a bit of a hack since ClientEnd doesn't expose its name easily,
		// but in the tests we can use the index as string if needed,
		// or just some dummy values because LabrpcKVClient uses the End directly.
		names[i] = "labrpc-peer"
	}
	return names
}
