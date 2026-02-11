package storage

// Iterator defines the interface for database iteration
type Iterator interface {
	Valid() bool
	Next()
	Key() []byte
	Value() []byte
	Error() error
	Close()
}

// Storage defines the common interface for key-value storage engines
type Storage interface {
	Get(key []byte) ([]byte, error)
	Put(key []byte, value []byte) error
	Delete(key []byte) error
	NewIterator(prefix []byte) Iterator
	Close() error
	Size() int64
}

// ShardStorage extends Storage with shard-aware operations
type ShardStorage interface {
	Storage
	GetShard(shardID int, key []byte) ([]byte, error)
	PutShard(shardID int, key []byte, value []byte) error
	DeleteShard(shardID int, key []byte) error
	NewShardIterator(shardID int) Iterator
}
