package storage_eng

// import (
// 	"math/rand"
// 	"time"

// 	"github.com/vmware/go-pmem-transaction/pmem"
// )

// type PMemKvStore struct {
// 	db     *SkipList
// 	dbPath string
// }

// func MakePMemKvStore(path string) (*PMemKvStore, error) {
// 	rand.Seed(time.Now().UTC().UnixNano()) // for skiplist random level
// 	var rptr *SkipList

// 	firstInit := pmem.Init("./list_database")
// 	if firstInit {
// 		rptr = (*SkipList)(pmem.New("dbRoot", rptr))
// 		rptr.Init()
// 	} else {
// 		rptr = (*SkipList)(pmem.Get("dbRoot", rptr))
// 	}
// 	return &PMemKvStore{
// 		db:     rptr,
// 		dbPath: path,
// 	}, nil
// }

// func (pmemSt *PMemKvStore) Put(k string, v string) error {
// 	pmemSt.db.Insert([]byte(k), []byte(v))
// 	return nil
// }

// func (pmemSt *PMemKvStore) Get(k string) (string, error) {
// 	elem := pmemSt.db.Find([]byte(k))
// 	return string(elem.Value), nil
// }

// func (pmemSt *PMemKvStore) Delete(k string) error {
// 	pmemSt.db.Delete([]byte(k))
// 	return nil
// }

// func (pmemSt *PMemKvStore) DumpPrefixKey(string) (map[string]string, error) {
// 	return nil, nil
// }

// func (pmemSt *PMemKvStore) PutBytesKv(k []byte, v []byte) error {
// 	return nil
// }

// func (pmemSt *PMemKvStore) DeleteBytesK(k []byte) error {
// 	return nil
// }

// func (pmemSt *PMemKvStore) GetBytesValue(k []byte) ([]byte, error) {
// 	return make([]byte, 0), nil
// }

// func (pmemSt *PMemKvStore) SeekPrefixLast(prefix string) ([]byte, []byte, error) {
// 	return make([]byte, 0), make([]byte, 0), nil
// }

// func (pmemSt *PMemKvStore) SeekPrefixFirst(prefix string) ([]byte, []byte, error) {
// 	return make([]byte, 0), make([]byte, 0), nil
// }

// func (pmemSt *PMemKvStore) DelPrefixKeys(prefix string) error {
// 	return nil
// }
