// MIT License

// Copyright (c) 2022 eraft dev group

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

package storage

import (
	"bytes"
	"encoding/binary"
	"io/ioutil"
	"os"
	"path"
	"testing"
)

func RemoveDir(in string) {
	dir, _ := ioutil.ReadDir(in)
	for _, d := range dir {
		os.RemoveAll(path.Join([]string{in, d.Name()}...))
	}
}

func TestPrefixRange(t *testing.T) {
	ldb, err := MakeLevelDBKvStore("./test_data")
	if err != nil {
		t.Log(err)
		return
	}

	prefixBytes := []byte{0x11, 0x11, 0x19, 0x96}
	for i := 0; i < 300; i++ {
		var outBuf bytes.Buffer
		outBuf.Write(prefixBytes)
		b := make([]byte, 8)
		binary.LittleEndian.PutUint64(b, uint64(i))
		outBuf.Write(b)
		t.Logf("write %v", outBuf.Bytes())
		ldb.PutBytesKv(outBuf.Bytes(), []byte{byte(i)})
	}

	idMax, err := ldb.SeekPrefixKeyIdMax(prefixBytes)
	if err != nil {
		t.Log(err)
		return
	}

	t.Logf("idMax -> %d", idMax)

	ldb.db.Close()
	RemoveDir("./test_data")
}
