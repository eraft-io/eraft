///////////////////////////////////////////////////////////////////////
// Copyright 2018-2019 VMware, Inc.
// SPDX-License-Identifier: BSD-2-Clause
///////////////////////////////////////////////////////////////////////

package redis

import (
	"bytes"
	"fmt"
	"os"
	"pmem/region"
	"runtime"
	"strconv"
	"testing"

	"github.com/vmware/go-pmem-transaction/transaction"
)

func TestQuicklistBasic(t *testing.T) {
	fmt.Println("Basic quicklist tests.")
	os.Remove("testquicklist")
	offset := runtime.PmallocInit("testquicklist", transaction.LOGSIZE, DATASIZE)
	region.InitMeta(offset, transaction.LOGSIZE, UUID)
	tx := transaction.NewLargeUndo()

	var (
		ql    *quicklist
		iter  *quicklistIter
		entry quicklistEntry
		val   interface{}
		count int
	)

	fmt.Println("Create empty list")
	tx.Begin()
	ql = quicklistCreate(tx)
	assertEqual(t, ql.verify(0, 0, 0, 0), true)
	tx.Abort()

	fmt.Println("Push to tail of empty list")
	tx.Begin()
	ql = quicklistCreate(tx)
	ql.PushTail(tx, []byte("hello"))
	assertEqual(t, ql.verify(1, 1, 1, 1), true)
	tx.Abort()

	fmt.Println("Push to head of empty list")
	tx.Begin()
	ql = quicklistCreate(tx)
	ql.PushHead(tx, []byte("hello"))
	assertEqual(t, ql.verify(1, 1, 1, 1), true)
	tx.Abort()

	fmt.Println("Pop empty list")
	tx.Begin()
	ql = quicklistCreate(tx)
	val = ql.Pop(tx, true)
	assertEqual(t, ql.verify(0, 0, 0, 0), true)
	assertEqual(t, val == nil, true)
	tx.Abort()

	fmt.Println("Pop 1 string from list")
	tx.Begin()
	ql = quicklistCreate(tx)
	ql.PushHead(tx, []byte("hello"))
	val = ql.Pop(tx, true)
	assertEqual(t, ql.verify(0, 0, 0, 0), true)
	assertEqual(t, val, []byte("hello"))
	tx.Abort()

	fmt.Println("Pop 1 number from list")
	tx.Begin()
	ql = quicklistCreate(tx)
	ql.PushHead(tx, []byte("55513"))
	val = ql.Pop(tx, true)
	assertEqual(t, ql.verify(0, 0, 0, 0), true)
	assertEqual(t, val, int64(55513))
	tx.Abort()

	fmt.Println("Pop 500 from 500 list")
	tx.Begin()
	ql = quicklistCreate(tx)
	for i := 0; i < 500; i++ {
		ql.PushHead(tx, []byte("hello"+strconv.Itoa(i)))
	}
	for i := 0; i < 500; i++ {
		val = ql.Pop(tx, false)
		assertEqual(t, val, []byte("hello"+strconv.Itoa(i)))
	}
	assertEqual(t, ql.verify(0, 0, 0, 0), true)
	tx.Abort()

	fmt.Println("Pop 5000 from 500 list")
	tx.Begin()
	ql = quicklistCreate(tx)
	for i := 0; i < 500; i++ {
		ql.PushHead(tx, []byte("hello"+strconv.Itoa(i)))
	}
	for i := 0; i < 5000; i++ {
		val = ql.Pop(tx, false)
		if i < 500 {
			assertEqual(t, val, []byte("hello"+strconv.Itoa(i)))
		} else {
			assertEqual(t, val, nil)
		}
	}
	assertEqual(t, ql.verify(0, 0, 0, 0), true)
	tx.Abort()

	fmt.Println("Iterate forward over 500 list")
	tx.Begin()
	ql = quicklistCreate(tx)
	ql.SetFill(tx, 32)
	for i := 0; i < 500; i++ {
		ql.PushHead(tx, []byte("hello"+strconv.Itoa(i)))
	}
	iter = ql.GetIterator(true)
	count = 0
	for iter.Next(&entry) {
		count++
		assertEqual(t, entry.value, []byte("hello"+strconv.Itoa(500-count)))
	}
	assertEqual(t, count, 500)
	assertEqual(t, ql.verify(16, 500, 20, 32), true)
	tx.Abort()

	fmt.Println("Iterate backward over 500 list")
	tx.Begin()
	ql = quicklistCreate(tx)
	ql.SetFill(tx, 32)
	for i := 0; i < 500; i++ {
		ql.PushHead(tx, []byte("hello"+strconv.Itoa(i)))
	}
	iter = ql.GetIterator(false)
	count = 0
	for iter.Next(&entry) {
		assertEqual(t, entry.value, []byte("hello"+strconv.Itoa(count)))
		count++
	}
	assertEqual(t, count, 500)
	assertEqual(t, ql.verify(16, 500, 20, 32), true)
	tx.Abort()

	fmt.Println("Insert before with 0 elements")
	tx.Begin()
	ql = quicklistCreate(tx)
	ql.Index(0, &entry)
	ql.InsertBefore(tx, &entry, []byte("abc"))
	assertEqual(t, ql.verify(1, 1, 1, 1), true)
	assertEqual(t, []byte("abc"), ql.Pop(tx, true))
	tx.Abort()

	fmt.Println("Insert after with 0 elements")
	tx.Begin()
	ql = quicklistCreate(tx)
	ql.Index(0, &entry)
	ql.InsertAfter(tx, &entry, []byte("abc"))
	assertEqual(t, ql.verify(1, 1, 1, 1), true)
	assertEqual(t, []byte("abc"), ql.Pop(tx, true))
	tx.Abort()

	fmt.Println("Insert after 1 element")
	tx.Begin()
	ql = quicklistCreate(tx)
	ql.PushHead(tx, []byte("hello"))
	ql.Index(0, &entry)
	ql.InsertAfter(tx, &entry, []byte("abc"))
	assertEqual(t, ql.verify(1, 2, 2, 2), true)
	assertEqual(t, []byte("hello"), ql.Pop(tx, true))
	assertEqual(t, []byte("abc"), ql.Pop(tx, true))
	tx.Abort()

	fmt.Println("Insert before 1 element")
	tx.Begin()
	ql = quicklistCreate(tx)
	ql.PushHead(tx, []byte("hello"))
	ql.Index(0, &entry)
	ql.InsertBefore(tx, &entry, []byte("abc"))
	assertEqual(t, ql.verify(1, 2, 2, 2), true)
	assertEqual(t, []byte("abc"), ql.Pop(tx, true))
	assertEqual(t, []byte("hello"), ql.Pop(tx, true))
	tx.Abort()

	fmt.Println("Insert once in elements while iterating at different fill")
	for f := -5; f < 12; f++ {
		tx.Begin()
		ql = quicklistNew(tx, f, 0) // TODO: compress not implemented
		ql.PushTail(tx, []byte("abc"))
		ql.SetFill(tx, 1)
		ql.PushTail(tx, []byte("def")) // force unique node
		ql.SetFill(tx, f)
		ql.PushTail(tx, []byte("bob"))
		ql.PushTail(tx, []byte("foo"))
		ql.PushTail(tx, []byte("zoo"))

		// insert "bar" before "bob" while iterating over list.
		iter = ql.GetIterator(true)
		for iter.Next(&entry) {
			if bytes.Equal(entry.value.([]byte), []byte("bob")) {
				ql.InsertBefore(tx, &entry, []byte("bar"))
				break
			}
		}
		ql.Index(0, &entry)
		assertEqual(t, []byte("abc"), entry.value)
		ql.Index(1, &entry)
		assertEqual(t, []byte("def"), entry.value)
		ql.Index(2, &entry)
		assertEqual(t, []byte("bar"), entry.value)
		ql.Index(3, &entry)
		assertEqual(t, []byte("bob"), entry.value)
		ql.Index(4, &entry)
		assertEqual(t, []byte("foo"), entry.value)
		ql.Index(5, &entry)
		assertEqual(t, []byte("zoo"), entry.value)
		tx.Abort()
	}

	fmt.Println("Insert before 250 new in middle of 500 elements at different fill")
	for f := -5; f < 1024; f++ {
		tx.Begin()
		ql = quicklistNew(tx, f, 0)
		for i := 0; i < 500; i++ {
			ql.PushTail(tx, []byte("hello"+strconv.Itoa(i)))
		}
		for i := 0; i < 250; i++ {
			ql.Index(250, &entry)
			ql.InsertBefore(tx, &entry, []byte("abc"+strconv.Itoa(i)))
		}
		assertEqual(t, ql.count, 750)
		if f == 32 {
			assertEqual(t, ql.verify(25, 750, 32, 20), true)
		}
		tx.Abort()
	}

	fmt.Println("Insert after 250 new in middle of 500 elements at different fill")
	for f := -5; f < 1024; f++ {
		tx.Begin()
		ql = quicklistNew(tx, f, 0)
		for i := 0; i < 500; i++ {
			ql.PushHead(tx, []byte("hello"+strconv.Itoa(i)))
		}
		for i := 0; i < 250; i++ {
			ql.Index(250, &entry)
			ql.InsertAfter(tx, &entry, []byte("abc"+strconv.Itoa(i)))
		}
		assertEqual(t, ql.count, 750)
		if f == 32 {
			assertEqual(t, ql.verify(26, 750, 20, 32), true)
		}
		tx.Abort()
	}

	fmt.Println("Index from 500 list at different fill")
	for f := -5; f < 512; f++ {
		tx.Begin()
		ql = quicklistNew(tx, f, 0)
		for i := 0; i < 500; i++ {
			ql.PushTail(tx, []byte("hello"+strconv.Itoa(i+1)))
		}
		ql.Index(1, &entry)
		assertEqual(t, entry.value, []byte("hello2"))
		ql.Index(200, &entry)
		assertEqual(t, entry.value, []byte("hello201"))
		ql.Index(-1, &entry)
		assertEqual(t, entry.value, []byte("hello500"))
		ql.Index(-2, &entry)
		assertEqual(t, entry.value, []byte("hello499"))
		ql.Index(-100, &entry)
		assertEqual(t, entry.value, []byte("hello401"))
		assertEqual(t, ql.Index(500, &entry), false)
		tx.Abort()
	}

	fmt.Println("Delete range empty list")
	tx.Begin()
	ql = quicklistNew(tx, -2, 0)
	ql.DelRange(tx, 5, 20)
	assertEqual(t, ql.verify(0, 0, 0, 0), true)
	tx.Abort()

	fmt.Println("Delete range of entire node in list of one node")
	tx.Begin()
	ql = quicklistNew(tx, -2, 0)
	for i := 0; i < 32; i++ {
		ql.PushHead(tx, []byte("hello"))
	}
	assertEqual(t, ql.verify(1, 32, 32, 32), true)
	ql.DelRange(tx, 0, 32)
	assertEqual(t, ql.verify(0, 0, 0, 0), true)
	tx.Abort()

	fmt.Println("Delete range of entire node with overflow counts")
	tx.Begin()
	ql = quicklistNew(tx, -2, 0)
	for i := 0; i < 32; i++ {
		ql.PushHead(tx, []byte("hello"))
	}
	assertEqual(t, ql.verify(1, 32, 32, 32), true)
	ql.DelRange(tx, 0, 128)
	assertEqual(t, ql.verify(0, 0, 0, 0), true)
	tx.Abort()

	fmt.Println("Delete middle 100 of 500 list")
	tx.Begin()
	ql = quicklistNew(tx, 32, 0)
	for i := 0; i < 500; i++ {
		ql.PushTail(tx, []byte("hello"))
	}
	assertEqual(t, ql.verify(16, 500, 32, 20), true)
	ql.DelRange(tx, 200, 100)
	assertEqual(t, ql.verify(14, 400, 32, 20), true)
	tx.Abort()

	fmt.Println("Delete negative 1 from 500 list")
	tx.Begin()
	ql = quicklistNew(tx, 32, 0)
	for i := 0; i < 500; i++ {
		ql.PushTail(tx, []byte("hello"))
	}
	assertEqual(t, ql.verify(16, 500, 32, 20), true)
	ql.DelRange(tx, -1, 1)
	assertEqual(t, ql.verify(16, 499, 32, 19), true)
	tx.Abort()

	fmt.Println("Delete negative 1 from 500 list with overflow counts")
	tx.Begin()
	ql = quicklistNew(tx, 32, 0)
	for i := 0; i < 500; i++ {
		ql.PushTail(tx, []byte("hello"))
	}
	assertEqual(t, ql.verify(16, 500, 32, 20), true)
	ql.DelRange(tx, -1, 128)
	assertEqual(t, ql.verify(16, 499, 32, 19), true)
	tx.Abort()

	fmt.Println("Delete negative 100 from 500 list")
	tx.Begin()
	ql = quicklistNew(tx, 32, 0)
	for i := 0; i < 500; i++ {
		ql.PushTail(tx, []byte("hello"))
	}
	assertEqual(t, ql.verify(16, 500, 32, 20), true)
	ql.DelRange(tx, -100, 100)
	assertEqual(t, ql.verify(13, 400, 32, 16), true)
	tx.Abort()

	fmt.Println("Delete -10 count 5 from 50 list")
	tx.Begin()
	ql = quicklistNew(tx, 32, 0)
	for i := 0; i < 50; i++ {
		ql.PushTail(tx, []byte("hello"))
	}
	assertEqual(t, ql.verify(2, 50, 32, 18), true)
	ql.DelRange(tx, -10, 5)
	assertEqual(t, ql.verify(2, 45, 32, 13), true)
	tx.Abort()

	fmt.Println("Numbers only list read")
	tx.Begin()
	ql = quicklistNew(tx, -2, 0)
	ql.PushTail(tx, []byte("1111"))
	ql.PushTail(tx, []byte("2222"))
	ql.PushTail(tx, []byte("3333"))
	ql.PushTail(tx, []byte("4444"))
	assertEqual(t, ql.verify(1, 4, 4, 4), true)
	ql.Index(0, &entry)
	assertEqual(t, entry.value, int64(1111))
	ql.Index(1, &entry)
	assertEqual(t, entry.value, int64(2222))
	ql.Index(2, &entry)
	assertEqual(t, entry.value, int64(3333))
	ql.Index(3, &entry)
	assertEqual(t, entry.value, int64(4444))
	assertEqual(t, ql.Index(4, &entry), false)
	ql.Index(-4, &entry)
	assertEqual(t, entry.value, int64(1111))
	ql.Index(-3, &entry)
	assertEqual(t, entry.value, int64(2222))
	ql.Index(-2, &entry)
	assertEqual(t, entry.value, int64(3333))
	ql.Index(-1, &entry)
	assertEqual(t, entry.value, int64(4444))
	assertEqual(t, ql.Index(-5, &entry), false)
	tx.Abort()

	fmt.Println("Numbers larger list read")
	tx.Begin()
	ql = quicklistNew(tx, 32, 0)
	nums := make([]int64, 5000)
	for i := 0; i < 5000; i++ {
		nums[i] = -5157318210846258176 + int64(i)
		ql.PushTail(tx, []byte(strconv.FormatInt(nums[i], 10)))
	}
	ql.PushTail(tx, []byte("xxxxxxxxxxxxxxxxxxxx"))
	for i := 0; i < 5000; i++ {
		ql.Index(i, &entry)
		assertEqual(t, entry.value, nums[i])
	}
	ql.Index(5000, &entry)
	assertEqual(t, entry.value, []byte("xxxxxxxxxxxxxxxxxxxx"))
	ql.ReplaceAtIndex(tx, 0, []byte("foo"))
	ql.ReplaceAtIndex(tx, -1, int64(111))
	ql.Index(0, &entry)
	assertEqual(t, entry.value, []byte("foo"))
	ql.Index(5000, &entry)
	assertEqual(t, entry.value, int64(111))
	assertEqual(t, ql.verify(157, 5001, 32, 9), true)
	tx.Abort()

	fmt.Println("lrem test at different fill")
	words := []string{"abc", "foo", "bar", "foobar", "foobared", "zap", "bar", "test", "foo"}
	result := []string{"abc", "foo", "foobar", "foobared", "zap", "test", "foo"}
	result2 := []string{"abc", "foo", "foobar", "foobared", "zap", "test"}
	for f := -5; f < 16; f++ {
		tx.Begin()
		ql = quicklistNew(tx, f, 0)
		for i := 0; i < 9; i++ {
			ql.PushTail(tx, []byte(words[i]))
		}
		// lrem 0 bar
		i := 0
		iter = ql.GetIterator(true)
		for iter.Next(&entry) {
			if entry.Compare([]byte("bar")) {
				iter.DelEntry(tx, &entry)
			}
			i++
		}
		iter = ql.GetIterator(true)
		i = 0
		for iter.Next(&entry) {
			assertEqual(t, entry.value, []byte(result[i]))
			i++
		}

		// lrem -2 foo
		ql.PushTail(tx, []byte("foo"))
		iter = ql.GetIterator(false)
		del := 2
		for iter.Next(&entry) {
			if entry.Compare([]byte("foo")) {
				iter.DelEntry(tx, &entry)
				del--
			}
			if del == 0 {
				break
			}
			i++
		}
		iter = ql.GetIterator(false)
		i = 0
		for iter.Next(&entry) {
			assertEqual(t, entry.value, []byte(result2[len(result2)-1-i]))
			i++
		}
		tx.Abort()
	}
}

func (ql *quicklist) verify(length, count, headcount, tailcount int) bool {
	match := true
	if length != ql.length {
		fmt.Println("quicklist length wrong! expected", length, "get", ql.length)
		match = false
	}

	if count != ql.count {
		fmt.Println("quicklist count wrong! expected", count, "get", ql.count)
		match = false
	}

	if count != ql.iterCount(true) {
		fmt.Println("quicklist count wrong! expected", count,
			"get from forward iterating", ql.iterCount(true))
		match = false
	}

	if count != ql.iterCount(false) {
		fmt.Println("quicklist count wrong! expected", count,
			"get from backward iterating", ql.iterCount(false))
		match = false
	}

	if ql.head != nil && int(ql.head.zl.entries) != headcount {
		fmt.Println("quicklist headcount wrong! expected", headcount,
			"get", ql.head.zl.entries)
		match = false
	}

	if ql.tail != nil && int(ql.tail.zl.entries) != tailcount {
		fmt.Println("quicklist tailcount wrong! expected", tailcount,
			"get", ql.tail.zl.entries)
		match = false
	}

	return match
}

func (ql *quicklist) iterCount(forward bool) int {
	iter := ql.GetIterator(forward)
	var entry quicklistEntry
	i := 0
	for iter.Next(&entry) {
		i++
	}
	return i
}
