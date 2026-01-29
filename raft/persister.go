package raft

//
// support for Raft and kvraft to save persistent
// Raft state (log &c) and k/v server snapshots.
//
// we will use the original persister.go to test your code for grading.
// so, while you can modify this code to help you debug, please
// test with the original before submitting.
//

import (
	"os"
	"sync"
)

type Persister struct {
	mu        sync.Mutex
	raftstate []byte
	snapshot  []byte
	path      string
}

func MakePersister() *Persister {
	return &Persister{}
}

func MakeFilePersister(path string) *Persister {
	ps := &Persister{path: path}
	ps.readFromDisk()
	return ps
}

func (ps *Persister) readFromDisk() {
	if ps.path == "" {
		return
	}
	state, err := os.ReadFile(ps.path + ".state")
	if err == nil {
		ps.raftstate = state
	}
	snapshot, err := os.ReadFile(ps.path + ".snapshot")
	if err == nil {
		ps.snapshot = snapshot
	}
}

func (ps *Persister) saveToDisk() {
	if ps.path == "" {
		return
	}
	os.WriteFile(ps.path+".state", ps.raftstate, 0644)
	os.WriteFile(ps.path+".snapshot", ps.snapshot, 0644)
}

func clone(orig []byte) []byte {
	x := make([]byte, len(orig))
	copy(x, orig)
	return x
}

func (ps *Persister) Copy() *Persister {
	ps.mu.Lock()
	defer ps.mu.Unlock()
	np := MakePersister()
	np.raftstate = ps.raftstate
	np.snapshot = ps.snapshot
	return np
}

func (ps *Persister) SaveRaftState(state []byte) {
	ps.mu.Lock()
	defer ps.mu.Unlock()
	ps.raftstate = clone(state)
	ps.saveToDisk()
}

func (ps *Persister) ReadRaftState() []byte {
	ps.mu.Lock()
	defer ps.mu.Unlock()
	return clone(ps.raftstate)
}

func (ps *Persister) RaftStateSize() int {
	ps.mu.Lock()
	defer ps.mu.Unlock()
	return len(ps.raftstate)
}

// Save both Raft state and K/V snapshot as a single atomic action,
// to help avoid them getting out of sync.
func (ps *Persister) SaveStateAndSnapshot(state []byte, snapshot []byte) {
	ps.mu.Lock()
	defer ps.mu.Unlock()
	ps.raftstate = clone(state)
	ps.snapshot = clone(snapshot)
	ps.saveToDisk()
}

func (ps *Persister) ReadSnapshot() []byte {
	ps.mu.Lock()
	defer ps.mu.Unlock()
	return clone(ps.snapshot)
}

func (ps *Persister) SnapshotSize() int {
	ps.mu.Lock()
	defer ps.mu.Unlock()
	return len(ps.snapshot)
}
