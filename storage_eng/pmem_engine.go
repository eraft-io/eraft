package storage_eng

import (
	"encoding/binary"
	"errors"
	"strings"
	"time"

	"github.com/gomodule/redigo/redis"
)

func newPool(addr string) *redis.Pool {
	return &redis.Pool{
		MaxIdle:     3,
		IdleTimeout: 240 * time.Second,
		// Dial or DialContext must be set. When both are set, DialContext takes precedence over Dial.
		Dial: func() (redis.Conn, error) { return redis.Dial("tcp", addr) },
	}
}

type PMemKvStore struct {
	dbUrl string
	pool  *redis.Pool
}

func MakePMemKvStore(url string) (*PMemKvStore, error) {
	return &PMemKvStore{
		dbUrl: url,
		pool:  newPool(url),
	}, nil
}

func (pmemSt *PMemKvStore) Close() {
	pmemSt.pool.Get().Close()
}

func (pmemSt *PMemKvStore) Put(k string, v string) error {
	return pmemSt.PutBytesKv([]byte(k), []byte(v))
}

func (pmemSt *PMemKvStore) FlushDB() {
	_, err := pmemSt.pool.Get().Do("FLUSHDB")
	if err != nil {
		panic(err)
	}
}

func (pmemSt *PMemKvStore) Get(k string) (string, error) {
	val, err := pmemSt.GetBytesValue([]byte(k))
	if err != nil {
		return "", err
	}
	return string(val), nil
}

func (pmemSt *PMemKvStore) Delete(k string) error {
	return pmemSt.DeleteBytesK([]byte(k))
}

func (pmemSt *PMemKvStore) DumpPrefixKey(prefix string) (map[string]string, error) {
	kvs := make(map[string]string, 0)
	conn := pmemSt.pool.Get()
	defer conn.Close()
	keys, err := redis.Strings(conn.Do("ZRANGEBYLEX", "ROOT_SET", "["+prefix, "+"))
	if err != nil {
		return nil, err
	}
	for _, k := range keys {
		if strings.HasPrefix(k, prefix) {
			val, err := pmemSt.Get(k)
			if err != nil {
				return nil, err
			}
			kvs[k] = val
		}
	}
	return kvs, nil
}

func (pmemSt *PMemKvStore) PutBytesKv(k []byte, v []byte) error {
	conn := pmemSt.pool.Get()
	defer conn.Close()
	_, err := conn.Do("SET", k, v)
	if err != nil {
		return err
	}
	_, err = conn.Do("ZADD", "ROOT_SET", 0, string(k))
	if err != nil {
		return err
	}
	return nil
}

func (pmemSt *PMemKvStore) DeleteBytesK(k []byte) error {
	conn := pmemSt.pool.Get()
	defer conn.Close()
	_, err := redis.Bytes(conn.Do("DEL", k))
	if err != nil {
		return err
	}
	return nil
}

func (pmemSt *PMemKvStore) GetBytesValue(k []byte) ([]byte, error) {
	conn := pmemSt.pool.Get()
	defer conn.Close()
	val, err := redis.Bytes(conn.Do("GET", k))
	if err != nil {
		return nil, err
	}
	return val, nil
}

func (pmemSt *PMemKvStore) SeekPrefixLast(prefix []byte) ([]byte, []byte, error) {

	return nil, nil, nil
}
func (pmemSt *PMemKvStore) SeekPrefixFirst(prefix string) ([]byte, []byte, error) {
	conn := pmemSt.pool.Get()
	defer conn.Close()
	keys, err := redis.ByteSlices(conn.Do("ZRANGEBYLEX", "ROOT_SET", "["+prefix, "+"))
	if err != nil {
		return []byte{}, []byte{}, err
	}
	firstVal, err := pmemSt.GetBytesValue(keys[0])
	if err != nil {
		return []byte{}, []byte{}, errors.New("seek not find key")
	}
	return keys[0], firstVal, nil
}

func (pmemSt *PMemKvStore) DelPrefixKeys(prefix string) error {
	return nil
}

func (pmemSt *PMemKvStore) SeekPrefixKeyIdMax(prefix []byte) (uint64, error) {
	var maxKeyId uint64
	maxKeyId = 0
	conn := pmemSt.pool.Get()
	defer conn.Close()
	keys, err := redis.ByteSlices(conn.Do("ZRANGEBYLEX", "ROOT_SET", "["+string(prefix), "+"))
	if err != nil {
		return 0, err
	}
	for _, k := range keys {
		if strings.HasPrefix(string(k), string(prefix)) {
			KeyId := binary.LittleEndian.Uint64(k[len(prefix):])
			if KeyId > maxKeyId {
				maxKeyId = KeyId
			}
		}
	}
	return maxKeyId, nil
}
