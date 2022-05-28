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

package configserver

import (
	"testing"

	"github.com/eraft-io/eraft/common"
	"github.com/eraft-io/eraft/raftcore"
	"github.com/eraft-io/eraft/storage_eng"
)

func TestRangeArr(t *testing.T) {

	var newBuckets [common.NBuckets]int

	newBuckets[0] = 2

	for k, v := range newBuckets {
		t.Logf("k -> %d, v -> %d", k, v)
	}
}

func TestAddGroups(t *testing.T) {
	newdbEng, err := storage_eng.MakeLevelDBKvStore("./conf_data/" + "/test")
	if err != nil {
		raftcore.PrintDebugLog("boot storage engine err!")
		panic(err)
	}
	memConfStm := NewMemConfigStm(newdbEng)
	for i := 0; i < 1000; i++ {
		conf, _ := memConfStm.Query(-1)
		t.Logf("%v %d", conf, i)
	}
}
