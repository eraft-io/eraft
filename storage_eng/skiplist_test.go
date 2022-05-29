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
//
package storage_eng

// import (
// 	"fmt"
// 	"math/rand"
// 	"testing"
// 	"time"

// 	"github.com/vmware/go-pmem-transaction/pmem"
// )

// func TestEngRun(t *testing.T) {
// 	rand.Seed(time.Now().UTC().UnixNano())
// 	firstInit := pmem.Init("./list_database")
// 	var rptr *SkipList
// 	if firstInit {
// 		// Create a new named object called dbRoot and point it to rptr
// 		rptr = (*SkipList)(pmem.New("dbRoot", rptr))
// 		rptr.Init()
// 		rptr.Insert([]byte{0x97, 0x98}, []byte("hello my engine1"))
// 		rptr.Insert([]byte{0x97, 0x99}, []byte("hello my engine2"))
// 		rptr.Insert([]byte{0x97, 0x97}, []byte("hello my engine3"))
// 	} else {
// 		// Retrieve the named object dbRoot
// 		rptr = (*SkipList)(pmem.Get("dbRoot", rptr))

// 		rptr.Delete([]byte{0x97, 0x98})
// 		for e := rptr.Front(); e != nil; e = e.Next() {
// 			fmt.Printf("%x \n", e.Key)
// 			fmt.Printf("%s \n", e.Value)
// 		}
// 	}
// }
