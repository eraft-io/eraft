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
// a skiplist implementation kv engine based on (AEP) persist memory
//
package storage_eng

// import (
// 	"bytes"
// 	"math/rand"

// 	"github.com/vmware/go-pmem-transaction/transaction"
// )

// const SKIPLIST_MAXLEVEL = 32
// const SKIPLIST_BRANCH = 4

// type skiplistLevel struct {
// 	forward *Element
// 	span    int
// }

// type Element struct {
// 	Key      []byte
// 	Value    []byte
// 	backward *Element
// 	level    []*skiplistLevel
// }

// func (e *Element) Next() *Element {
// 	return e.level[0].forward
// }

// func (e *Element) Prev() *Element {
// 	return e.backward
// }

// func newElement(level int, k []byte, v []byte) *Element {
// 	elem := pnew(Element)
// 	slLevels := pmake([]*skiplistLevel, level)
// 	for i := 0; i < level; i++ {
// 		slLevels[i] = pnew(skiplistLevel)
// 	}
// 	tx := transaction.NewUndoTx()
// 	tx.Begin()
// 	elem.Key = pmake([]byte, len(k))
// 	for i := range elem.Key {
// 		elem.Key[i] = k[i]
// 	}
// 	elem.Value = pmake([]byte, len(v))
// 	for i := range elem.Value {
// 		elem.Value[i] = v[i]
// 	}
// 	elem.backward = nil
// 	elem.level = slLevels

// 	tx.End()
// 	transaction.Release(tx)

// 	return elem
// }

// func randomLevel() int {
// 	level := 1
// 	for (rand.Int31()&0xFFFF)%SKIPLIST_BRANCH == 0 {
// 		level += 1
// 	}

// 	if level < SKIPLIST_MAXLEVEL {
// 		return level
// 	} else {
// 		return SKIPLIST_MAXLEVEL
// 	}
// }

// type SkipList struct {
// 	header *Element
// 	tail   *Element
// 	update []*Element
// 	rank   []int
// 	length int
// 	level  int
// }

// func (rptr *SkipList) Init() {
// 	tx := transaction.NewUndoTx()
// 	tx.Begin()
// 	rptr.header = newElement(SKIPLIST_MAXLEVEL, nil, nil)
// 	rptr.tail = nil
// 	rptr.update = pmake([]*Element, SKIPLIST_MAXLEVEL)
// 	rptr.rank = pmake([]int, SKIPLIST_MAXLEVEL)
// 	rptr.length = 0
// 	rptr.level = 1
// 	tx.End()
// 	transaction.Release(tx)
// }

// func (sl *SkipList) Insert(key []byte, val []byte) *Element {
// 	tx := transaction.NewUndoTx()
// 	tx.Begin()
// 	x := sl.header
// 	for i := sl.level - 1; i >= 0; i-- {
// 		if i == sl.level-1 {
// 			sl.rank[i] = 0
// 		} else {
// 			sl.rank[i] = sl.rank[i+1]
// 		}
// 		for x.level[i].forward != nil && bytes.Compare(x.level[i].forward.Key, key) < 0 {
// 			sl.rank[i] += x.level[i].span
// 			x = x.level[i].forward
// 		}
// 		sl.update[i] = x
// 	}

// 	level := randomLevel()
// 	if level > sl.level {
// 		for i := sl.level; i < level; i++ {
// 			sl.rank[i] = 0
// 			sl.update[i] = sl.header
// 			sl.update[i].level[i].span = sl.length
// 		}
// 		sl.level = level
// 	}

// 	x = newElement(level, key, val)
// 	for i := 0; i < level; i++ {
// 		x.level[i].forward = sl.update[i].level[i].forward
// 		sl.update[i].level[i].forward = x

// 		// update span covered by update[i] as x is inserted here
// 		x.level[i].span = sl.update[i].level[i].span - sl.rank[0] + sl.rank[i]
// 		sl.update[i].level[i].span = sl.rank[0] - sl.rank[i] + 1
// 	}

// 	// increment span for untouched levels
// 	for i := level; i < sl.level; i++ {
// 		sl.update[i].level[i].span++
// 	}

// 	if sl.update[0] == sl.header {
// 		x.backward = nil
// 	} else {
// 		x.backward = sl.update[0]
// 	}
// 	if x.level[0].forward != nil {
// 		x.level[0].forward.backward = x
// 	} else {
// 		sl.tail = x
// 	}
// 	sl.length++

// 	tx.End()
// 	transaction.Release(tx)
// 	return x
// }

// func (sl *SkipList) find(key []byte) *Element {
// 	x := sl.header
// 	for i := sl.level - 1; i >= 0; i-- {
// 		for x.level[i].forward != nil && bytes.Compare(x.level[i].forward.Key, key) < 0 {
// 			x = x.level[i].forward
// 		}
// 		sl.update[i] = x
// 	}

// 	return x.level[0].forward
// }

// func (sl *SkipList) Front() *Element {
// 	return sl.header.level[0].forward
// }

// func (sl *SkipList) Back() *Element {
// 	return sl.tail
// }

// func (sl *SkipList) Len() int {
// 	return sl.length
// }

// func (sl *SkipList) Find(key []byte) *Element {
// 	x := sl.find(key)
// 	if x != nil && bytes.Compare(key, x.Key) == 0 {
// 		return x
// 	}
// 	return nil
// }

// func (sl *SkipList) deleteElement(e *Element, update []*Element) {
// 	tx := transaction.NewUndoTx()
// 	tx.Begin()
// 	for i := 0; i < sl.level; i++ {
// 		if update[i].level[i].forward == e {
// 			update[i].level[i].span += e.level[i].span - 1
// 			update[i].level[i].forward = e.level[i].forward
// 		} else {
// 			update[i].level[i].span -= 1
// 		}
// 	}

// 	if e.level[0].forward != nil {
// 		e.level[0].forward.backward = e.backward
// 	} else {
// 		sl.tail = e.backward
// 	}

// 	for sl.level > 1 && sl.header.level[sl.level-1].forward == nil {
// 		sl.level--
// 	}
// 	sl.length--
// 	tx.End()
// 	transaction.Release(tx)
// }

// func (sl *SkipList) Delete(val []byte) interface{} {
// 	x := sl.find(val)
// 	if x != nil && bytes.Compare(val, x.Key) == 0 {
// 		sl.deleteElement(x, sl.update)
// 		return x.Key
// 	}
// 	return nil
// }
