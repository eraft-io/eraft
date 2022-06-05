package storage_eng

import "testing"

func TestPmemEngGetSet(t *testing.T) {
	pmemSt, err := MakePMemKvStore("127.0.0.1:6379")
	if err != nil {
		panic(err)
	}
	if err := pmemSt.Put("testkey", "testval"); err != nil {
		panic(err)
	}
	val, err := pmemSt.Get("testkey")
	if err != nil {
		panic(err)
	}
	t.Log("got testkey -> ", val)
	pmemSt.FlushDB()
}

func TestPMemDumpPrefixKvs(t *testing.T) {
	pmemSt, err := MakePMemKvStore("127.0.0.1:6379")
	if err != nil {
		panic(err)
	}
	if err := pmemSt.Put("testa", "testvala"); err != nil {
		panic(err)
	}
	if err := pmemSt.Put("testb", "testvalb"); err != nil {
		panic(err)
	}
	if err := pmemSt.Put("testc", "testvalc"); err != nil {
		panic(err)
	}
	kvs, err := pmemSt.DumpPrefixKey("test")
	if err != nil {
		panic(err)
	}
	t.Logf("%v", kvs)

	fk, fv, err := pmemSt.SeekPrefixFirst("test")
	if err != nil {
		panic(err)
	}
	t.Logf("fk %s, fv %s", string(fk), string(fv))
	pmemSt.FlushDB()
}

func TestPMemPrefixMaxId(t *testing.T) {
	pmemSt, err := MakePMemKvStore("127.0.0.1:6379")
	if err != nil {
		panic(err)
	}
	if err := pmemSt.Put(string([]byte{0x00, 0x09, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}), "testval1"); err != nil {
		panic(err)
	}
	if err := pmemSt.Put(string([]byte{0x00, 0x09, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}), "testval2"); err != nil {
		panic(err)
	}
	if err := pmemSt.Put(string([]byte{0x00, 0x09, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}), "testval3"); err != nil {
		panic(err)
	}
	idx, err := pmemSt.SeekPrefixKeyIdMax([]byte{0x00, 0x09, 0x00, 0x00})
	if err != nil {
		panic(err)
	}
	t.Log(idx)
	pmemSt.FlushDB()
}

func TestPMemPrefixFirst(t *testing.T) {
	pmemSt, err := MakePMemKvStore("127.0.0.1:6379")
	if err != nil {
		panic(err)
	}
	if err := pmemSt.Put(string([]byte{0x00, 0x09, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}), "testval1"); err != nil {
		panic(err)
	}
	if err := pmemSt.Put(string([]byte{0x00, 0x09, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}), "testval2"); err != nil {
		panic(err)
	}
	if err := pmemSt.Put(string([]byte{0x00, 0x09, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}), "testval3"); err != nil {
		panic(err)
	}
	k, v, err := pmemSt.SeekPrefixFirst(string([]byte{0x00, 0x09, 0x00, 0x00}))
	if err != nil {
		panic(err)
	}
	t.Logf("%v", k)
	t.Logf("%v", v)
	pmemSt.FlushDB()
}
