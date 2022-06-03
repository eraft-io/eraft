package storage_eng

import (
	"log"

	"github.com/gomodule/redigo/redis"
)

type PMemKvStore struct {
	dbUrl  string
	dbConn redis.Conn
}

// "redis://127.0.0.1:6379"
func MakePMemKvStore(url string) (*PMemKvStore, error) {
	c, err := redis.Dial("tcp", url)
	if err != nil {
		log.Fatalln(err)
	}
	return &PMemKvStore{
		dbUrl:  url,
		dbConn: c,
	}, nil
}

func (pmemSt *PMemKvStore) Close() {
	pmemSt.dbConn.Close()
}

func (pmemSt *PMemKvStore) Put(k string, v string) error {
	_, err := pmemSt.dbConn.Do("SET", k, v)
	if err != nil {
		return err
	}
	return nil
}

func (pmemSt *PMemKvStore) Get(k string) (string, error) {
	val, err := redis.String(pmemSt.dbConn.Do("GET", k))
	if err != nil {
		return "", err
	}
	return val, nil
}

func (pmemSt *PMemKvStore) Delete(k string) error {
	return nil
}

func (pmemSt *PMemKvStore) DumpPrefixKey(string) (map[string]string, error) {
	return nil, nil
}
func (pmemSt *PMemKvStore) PutBytesKv(k []byte, v []byte) error {
	return nil
}

func (pmemSt *PMemKvStore) DeleteBytesK(k []byte) error {
	return nil
}

func (pmemSt *PMemKvStore) GetBytesValue(k []byte) ([]byte, error) {
	return nil, nil
}

func (pmemSt *PMemKvStore) SeekPrefixLast(prefix []byte) ([]byte, []byte, error) {
	return nil, nil, nil
}
func (pmemSt *PMemKvStore) SeekPrefixFirst(prefix string) ([]byte, []byte, error) {
	return nil, nil, nil

}

func (pmemSt *PMemKvStore) DelPrefixKeys(prefix string) error {
	return nil
}
func (pmemSt *PMemKvStore) SeekPrefixKeyIdMax(prefix []byte) (uint64, error) {
	return 0, nil
}
