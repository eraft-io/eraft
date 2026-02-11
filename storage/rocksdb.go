package storage

import (
	"os"
	"path/filepath"

	"github.com/linxGnu/grocksdb"
)

// RocksDBStorage implements Storage interface using RocksDB
type RocksDBStorage struct {
	db   *grocksdb.DB
	path string
	opts *grocksdb.Options
}

// rocksDBIterator wraps grocksdb.Iterator to implement Iterator interface
type rocksDBIterator struct {
	iter *grocksdb.Iterator
}

func (it *rocksDBIterator) Valid() bool {
	return it.iter.Valid()
}

func (it *rocksDBIterator) Next() {
	it.iter.Next()
}

func (it *rocksDBIterator) Key() []byte {
	return it.iter.Key().Data()
}

func (it *rocksDBIterator) Value() []byte {
	return it.iter.Value().Data()
}

func (it *rocksDBIterator) Error() error {
	return it.iter.Err()
}

func (it *rocksDBIterator) Close() {
	it.iter.Close()
}

// DefaultRocksDBOptions returns optimized RocksDB options for eRaft
func DefaultRocksDBOptions() *grocksdb.Options {
	opts := grocksdb.NewDefaultOptions()
	opts.SetCreateIfMissing(true)

	// Configure compression
	opts.SetCompression(grocksdb.SnappyCompression)

	// Configure write buffer
	opts.SetWriteBufferSize(64 * 1024 * 1024) // 64MB
	opts.SetMaxWriteBufferNumber(3)
	opts.SetMinWriteBufferNumberToMerge(1)

	// Configure level target sizes
	opts.SetMaxBytesForLevelBase(512 * 1024 * 1024) // 512MB
	opts.SetMaxBytesForLevelMultiplier(10)

	// Configure concurrent compaction threads
	opts.SetMaxBackgroundCompactions(4)
	opts.SetMaxBackgroundFlushes(2)

	// Configure block cache
	bbto := grocksdb.NewDefaultBlockBasedTableOptions()
	bbto.SetBlockCache(grocksdb.NewLRUCache(512 * 1024 * 1024)) // 512MB cache
	opts.SetBlockBasedTableFactory(bbto)

	return opts
}

// NewRocksDBStorage creates a new RocksDB storage instance
func NewRocksDBStorage(path string, opts *grocksdb.Options) (*RocksDBStorage, error) {
	if opts == nil {
		opts = DefaultRocksDBOptions()
	}

	db, err := grocksdb.OpenDb(opts, path)
	if err != nil {
		return nil, err
	}

	return &RocksDBStorage{
		db:   db,
		path: path,
		opts: opts,
	}, nil
}

func (s *RocksDBStorage) Get(key []byte) ([]byte, error) {
	ro := grocksdb.NewDefaultReadOptions()
	defer ro.Destroy()

	slice, err := s.db.Get(ro, key)
	if err != nil {
		return nil, err
	}
	defer slice.Free()

	if !slice.Exists() {
		return nil, nil
	}

	// Copy data since slice will be freed
	data := make([]byte, slice.Size())
	copy(data, slice.Data())
	return data, nil
}

func (s *RocksDBStorage) Put(key []byte, value []byte) error {
	wo := grocksdb.NewDefaultWriteOptions()
	defer wo.Destroy()

	return s.db.Put(wo, key, value)
}

func (s *RocksDBStorage) Delete(key []byte) error {
	wo := grocksdb.NewDefaultWriteOptions()
	defer wo.Destroy()

	return s.db.Delete(wo, key)
}

func (s *RocksDBStorage) NewIterator(prefix []byte) Iterator {
	ro := grocksdb.NewDefaultReadOptions()

	var iter *grocksdb.Iterator
	if prefix != nil {
		iter = s.db.NewIterator(ro)
		iter.Seek(prefix)
	} else {
		iter = s.db.NewIterator(ro)
		iter.SeekToFirst()
	}

	return &rocksDBIterator{iter: iter}
}

func (s *RocksDBStorage) Close() error {
	s.db.Close()
	return nil
}

func (s *RocksDBStorage) Size() int64 {
	var size int64
	filepath.Walk(s.path, func(_ string, info os.FileInfo, err error) error {
		if err != nil {
			return err
		}
		if !info.IsDir() {
			size += info.Size()
		}
		return nil
	})
	return size
}

// RocksDBShardStorage implements ShardStorage interface
type RocksDBShardStorage struct {
	*RocksDBStorage
}

// NewRocksDBShardStorage creates a new RocksDB shard storage instance
func NewRocksDBShardStorage(path string, opts *grocksdb.Options) (*RocksDBShardStorage, error) {
	storage, err := NewRocksDBStorage(path, opts)
	if err != nil {
		return nil, err
	}
	return &RocksDBShardStorage{RocksDBStorage: storage}, nil
}

func shardKey(shardID int, key []byte) []byte {
	prefix := []byte("s_")
	shardIDBytes := []byte(string(rune(shardID)))
	separator := []byte("_")

	result := make([]byte, 0, len(prefix)+len(shardIDBytes)+len(separator)+len(key))
	result = append(result, prefix...)
	result = append(result, shardIDBytes...)
	result = append(result, separator...)
	result = append(result, key...)
	return result
}

func (s *RocksDBShardStorage) GetShard(shardID int, key []byte) ([]byte, error) {
	return s.Get(shardKey(shardID, key))
}

func (s *RocksDBShardStorage) PutShard(shardID int, key []byte, value []byte) error {
	return s.Put(shardKey(shardID, key), value)
}

func (s *RocksDBShardStorage) DeleteShard(shardID int, key []byte) error {
	return s.Delete(shardKey(shardID, key))
}

func (s *RocksDBShardStorage) NewShardIterator(shardID int) Iterator {
	prefix := shardKey(shardID, []byte{})
	return s.NewIterator(prefix)
}
