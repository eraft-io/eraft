package raft

import (
	"os"
	"path/filepath"
	"sync"
)

type PersisterOptions struct {
	OnFs     bool   // 是否启用磁盘持久化
	RootPath string // 存储目录（如 "/tmp/raft-1"）
}

type Persister struct {
	mu        sync.Mutex
	raftstate []byte
	snapshot  []byte
	opts      PersisterOptions
}

func MakePersister(opts *PersisterOptions) *Persister {
	ps := &Persister{}
	if opts != nil {
		ps.opts = *opts
	}

	// 如果启用了磁盘持久化，尝试从磁盘加载已有状态
	if ps.opts.OnFs && ps.opts.RootPath != "" {
		ps.loadFromDisk()
	}
	return ps
}

func clone(orig []byte) []byte {
	if orig == nil {
		return nil
	}
	x := make([]byte, len(orig))
	copy(x, orig)
	return x
}

// 原子地将数据写入文件（先写临时文件，再 rename）
func atomicWriteFile(filename string, data []byte) error {
	tmpFile := filename + ".tmp"
	f, err := os.Create(tmpFile)
	if err != nil {
		return err
	}
	defer f.Close()

	_, err = f.Write(data)
	if err != nil {
		os.Remove(tmpFile)
		return err
	}

	err = f.Sync()
	if err != nil {
		os.Remove(tmpFile)
		return err
	}

	return os.Rename(tmpFile, filename)
}

// 从磁盘加载 raftstate 和 snapshot
func (ps *Persister) loadFromDisk() {
	raftFile := filepath.Join(ps.opts.RootPath, "raftstate")
	snapFile := filepath.Join(ps.opts.RootPath, "snapshot")

	// 创建目录（如果不存在）
	os.MkdirAll(ps.opts.RootPath, 0755)

	raftData, _ := os.ReadFile(raftFile)
	snapData, _ := os.ReadFile(snapFile)

	ps.raftstate = clone(raftData)
	ps.snapshot = clone(snapData)
}

// SaveRaftState 将 Raft 状态写入内存并持久化到磁盘（如果启用）
func (ps *Persister) SaveRaftState(state []byte) {
	ps.mu.Lock()
	defer ps.mu.Unlock()
	ps.raftstate = clone(state)

	if ps.opts.OnFs && ps.opts.RootPath != "" {
		raftFile := filepath.Join(ps.opts.RootPath, "raftstate")
		atomicWriteFile(raftFile, ps.raftstate)
	}
}

// ReadRaftState 返回 Raft 状态的副本
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

// SaveStateAndSnapshot 原子地保存 Raft 状态和快照（内存 + 磁盘）
func (ps *Persister) SaveStateAndSnapshot(state []byte, snapshot []byte) {
	ps.mu.Lock()
	defer ps.mu.Unlock()
	ps.raftstate = clone(state)
	ps.snapshot = clone(snapshot)

	if ps.opts.OnFs && ps.opts.RootPath != "" {
		raftFile := filepath.Join(ps.opts.RootPath, "raftstate")
		snapFile := filepath.Join(ps.opts.RootPath, "snapshot")

		// 先写快照，再写 raftstate（顺序不重要，但需都成功）
		// 更严格的原子性需要日志或事务，这里简化处理
		err1 := atomicWriteFile(snapFile, ps.snapshot)
		err2 := atomicWriteFile(raftFile, ps.raftstate)
		// 可选：记录错误日志（此处省略）
		_ = err1
		_ = err2
	}
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

// Copy 用于测试，复制当前状态（不复制磁盘路径）
func (ps *Persister) Copy() *Persister {
	ps.mu.Lock()
	defer ps.mu.Unlock()
	np := MakePersister(nil) // 不启用磁盘
	np.raftstate = clone(ps.raftstate)
	np.snapshot = clone(ps.snapshot)
	return np
}
