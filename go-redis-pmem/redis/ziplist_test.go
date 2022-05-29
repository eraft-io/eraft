///////////////////////////////////////////////////////////////////////
// Copyright 2018-2019 VMware, Inc.
// SPDX-License-Identifier: BSD-2-Clause
///////////////////////////////////////////////////////////////////////

package redis

import (
	"fmt"
	"os"
	"pmem/region"
	"runtime"
	"testing"

	"github.com/vmware/go-pmem-transaction/transaction"
)

func TestZiplistBasic(t *testing.T) {
	fmt.Println("Basic index/get tests.")
	os.Remove("testziplist")
	offset := runtime.PmallocInit("testziplist", transaction.LOGSIZE, DATASIZE)
	region.InitMeta(offset, transaction.LOGSIZE, UUID)
	tx := transaction.NewUndo()
	tx.Begin()
	zl, list := createList(tx)
	zl2, list2 := createList(tx)
	tx.Commit()

	var p int
	var v interface{}

	fmt.Println("Get element at first index.")
	p = zl.Index(0)
	v = zl.Get(p)
	assertEqual(t, v, list[0])

	fmt.Println("Get element at second index.")
	p = zl.Index(1)
	v = zl.Get(p)
	assertEqual(t, v, list[1])

	fmt.Println("Get element at last index.")
	p = zl.Index(3)
	v = zl.Get(p)
	assertEqual(t, v, list[3])

	fmt.Println("Get element out of range.")
	p = zl.Index(4)
	v = zl.Get(p)
	assertEqual(t, p, -1)
	assertEqual(t, v, nil)

	fmt.Println("Get element at index -1 (last element).")
	p = zl.Index(-1)
	v = zl.Get(p)
	assertEqual(t, v, list[3])

	fmt.Println("Get element at index -4 (first element).")
	p = zl.Index(-4)
	v = zl.Get(p)
	assertEqual(t, v, list[0])

	fmt.Println("Get element at index -5 (reverse out of range).")
	p = zl.Index(-5)
	v = zl.Get(p)
	assertEqual(t, p, -1)
	assertEqual(t, v, nil)

	var result []interface{}
	fmt.Println("Iterate list from 0 to end.")
	p = 0
	v = zl.Get(p)
	result = nil
	for v != nil {
		result = append(result, v)
		p = zl.Next(p)
		v = zl.Get(p)
	}
	assertEqual(t, result, list)

	fmt.Println("Iterate list from 1 to end.")
	p = zl.Index(1)
	v = zl.Get(p)
	result = nil
	for v != nil {
		result = append(result, v)
		p = zl.Next(p)
		v = zl.Get(p)
	}
	assertEqual(t, result, list[1:])

	fmt.Println("Iterate list from 2 to end.")
	p = zl.Index(2)
	v = zl.Get(p)
	result = nil
	for v != nil {
		result = append(result, v)
		p = zl.Next(p)
		v = zl.Get(p)
	}
	assertEqual(t, result, list[2:])

	fmt.Println("Iterate starting out of range.")
	p = zl.Index(4)
	v = zl.Get(p)
	result = nil
	for v != nil {
		result = append(result, v)
		p = zl.Next(p)
		v = zl.Get(p)
	}
	assertEqual(t, len(result), 0)

	fmt.Println("Iterate from back to front.")
	p = zl.Index(-1)
	v = zl.Get(p)
	result = nil
	for v != nil {
		result = append([]interface{}{v}, result...)
		p = zl.Prev(p)
		v = zl.Get(p)
	}
	assertEqual(t, result, list)

	fmt.Println("Iterate from back to front, deleting all elements.")
	tx.Begin()
	p = zl.Index(-1)
	v = zl.Get(p)
	i := 0
	for v != nil {
		i++
		assertEqual(t, v, list[len(list)-i])
		zl.Delete(tx, p)
		p = zl.Prev(p)
		v = zl.Get(p)
	}
	assertEqual(t, p, -1)
	assertEqual(t, zl.Len(), 0)
	tx.Abort() // abort changes made to zl

	fmt.Println("Delete inclusive range 0,0.")
	tx.Begin()
	zl.DeleteRange(tx, 0, 1)
	assertEqual(t, zl.entries, uint(3))
	assertEqual(t, zl.Get(0), list[1])
	assertEqual(t, zl.Find(list[1], 0), 0)
	p = zl.Index(-1)
	v = zl.Get(p)
	assertEqual(t, v, list[len(list)-1])
	tx.Abort() // abort changes made to zl

	fmt.Println("Delete inclusive range 0,1.")
	tx.Begin()
	zl.DeleteRange(tx, 0, 2)
	assertEqual(t, zl.entries, uint(2))
	assertEqual(t, zl.Get(0), list[2])
	assertEqual(t, zl.Find(list[0], 0), -1)
	assertEqual(t, zl.Find(list[2], 0), 0)
	p = zl.Index(-1)
	v = zl.Get(p)
	assertEqual(t, v, list[len(list)-1])
	tx.Abort() // abort changes made to zl

	fmt.Println("Delete inclusive range 1,2.")
	tx.Begin()
	zl.DeleteRange(tx, 1, 2)
	assertEqual(t, zl.entries, uint(2))
	assertEqual(t, zl.Get(0), list[0])
	assertEqual(t, zl.Find(list[1], 0), -1)
	assertEqual(t, zl.Find(list[0], 0), 0)
	p = zl.Index(-1)
	v = zl.Get(p)
	assertEqual(t, v, list[len(list)-1])
	tx.Abort() // abort changes made to zl

	fmt.Println("Delete with start index out of range.")
	tx.Begin()
	zl.DeleteRange(tx, 5, 1)
	assertEqual(t, zl.entries, uint(4))
	assertEqual(t, zl.Get(0), list[0])
	assertEqual(t, zl.Find(list[0], 0), 0)
	p = zl.Index(1)
	v = zl.Get(p)
	assertEqual(t, v, list[1])
	tx.Abort() // abort changes made to zl

	fmt.Println("Delete with num overflow.")
	tx.Begin()
	zl.DeleteRange(tx, 1, 5)
	assertEqual(t, zl.entries, uint(1))
	assertEqual(t, zl.Find(list[1], 0), -1)
	assertEqual(t, zl.Find(list[0], 0), 0)
	tx.Abort() // abort changes made to zl

	fmt.Println("Merge test.")
	tx.Begin()
	zl3 := ziplistNew(tx)
	zl4 := ziplistNew(tx)
	zl3.Merge(tx, zl4)
	assertEqual(t, zl3.Len(), 0)
	zl.Merge(tx, zl2)
	assertEqual(t, zl.entries, uint(8))
	tmplist := append(list, list2...)
	for i := 0; i < 8; i++ {
		p = zl.Index(i)
		v = zl.Get(p)
		assertEqual(t, v, tmplist[i])
	}
	p = zl.Index(-1)
	v = zl.Get(p)
	assertEqual(t, v, tmplist[7])
	tx.Abort()
}

func createList(tx transaction.TX) (*ziplist, []interface{}) {
	zl := ziplistNew(tx)
	zl.Push(tx, []byte("foo"), false)
	zl.Push(tx, []byte("quux"), false)
	zl.Push(tx, []byte("hello"), true)
	zl.Push(tx, int64(1024), false)
	list := []interface{}{[]byte("hello"), []byte("foo"), []byte("quux"), int64(1024)}
	return zl, list
}
