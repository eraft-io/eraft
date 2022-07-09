// Copyright [2022] [WellWood] [wellwood-x@googlegroups.com]

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

// 	http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package engine

import (
	"testing"

	eng "github.com/eraft-io/eraft/pkg/engine"
	"github.com/stretchr/testify/assert"
)

func TestPutGetLeveldb(t *testing.T) {
	ldb := eng.KvStoreFactory("leveldb", "./test_data")
	keyBytes := []byte{0x11, 0x11, 0x19, 0x96}
	valBytes := []byte{0x11, 0x11, 0x19, 0x97}
	if err := ldb.Put(keyBytes, valBytes); err != nil {
		panic(err.Error())
	}
	gotBytes, err := ldb.Get(keyBytes)
	if err != nil {
		panic(err)
	}
	assert.Equal(t, valBytes, gotBytes)
	RemoveDir("./test_data")
}
