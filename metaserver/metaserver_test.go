//
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
//

package metaserver

import (
	"testing"

	"github.com/eraft-io/eraft/common"
	"github.com/eraft-io/eraft/raftcore"
	storage_eng "github.com/eraft-io/eraft/storage"
)

func TestRangeArr(t *testing.T) {
	var new_buckets [common.NBuckets]int
	new_buckets[0] = 2
	for k, v := range new_buckets {
		t.Logf("k -> %d, v -> %d", k, v)
	}
}

func TestAddGroups(t *testing.T) {
	new_db_eng, err := storage_eng.MakeLevelDBKvStore("./conf_data/" + "/test")
	if err != nil {
		raftcore.PrintDebugLog("boot storage engine err!")
		panic(err)
	}
	mem_conf_stm := NewMemConfigStm(new_db_eng)
	for i := 0; i < 1000; i++ {
		conf, _ := mem_conf_stm.Query(-1)
		t.Logf("%v %d", conf, i)
	}
}
