package storage_eng

type PMemKvStore struct {
	// db     *SkipList
	dbPath string
}

func MakePMemKvStore(path string) (*PMemKvStore, error) {

	return nil, nil
}

func (pmemSt *PMemKvStore) Put(string, string) error {
	return nil
}

func (pmemSt *PMemKvStore) Get(string) (string, error) {
	return "", nil
}
func (pmemSt *PMemKvStore) Delete(string) error {
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
	return make([]byte, 0), nil
}
func (pmemSt *PMemKvStore) SeekPrefixLast(prefix string) ([]byte, []byte, error) {
	return make([]byte, 0), make([]byte, 0), nil
}
func (pmemSt *PMemKvStore) SeekPrefixFirst(prefix string) ([]byte, []byte, error) {
	return make([]byte, 0), make([]byte, 0), nil
}
func (pmemSt *PMemKvStore) DelPrefixKeys(prefix string) error {
	return nil
}
