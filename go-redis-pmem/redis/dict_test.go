///////////////////////////////////////////////////////////////////////
// Copyright 2018-2019 VMware, Inc.
// SPDX-License-Identifier: BSD-2-Clause
///////////////////////////////////////////////////////////////////////

package redis

import (
	"bytes"
	"fmt"
	"pmem/heap"
	"reflect"
	"runtime/debug"
	"strconv"
	"testing"
	"time"

	"github.com/vmware/go-pmem-transaction/transaction"
)

var d *dict

func TestServer(t *testing.T) {
	s := new(server)
	go s.Start()
	time.Sleep(180 * time.Second)

	conn := getClient()
	conn.Write([]byte("*3\r\n$3\r\nSET\r\n$3\r\nfoo\r\n$3\r\nbar\r\n"))
	time.Sleep(1 * time.Second)
	undoTx := transaction.NewUndo()
	fmt.Println(s.db.lookupKeyRead(undoTx, []byte("foo")))
	transaction.Release(undoTx)
}

func setup() transaction.TX {
	logSlice := make([]byte, transaction.LOGSIZE)
	heapSlice := make([]byte, 100000000)
	transaction.Init(logSlice)
	undoTx := transaction.NewUndo()
	heap.Init(undoTx, heapSlice, 100000000)
	d = NewDict(undoTx, 4, 1)
	return undoTx
}

func assertEqual(t *testing.T, actual, expected interface{}) {
	if expected == nil || actual == nil {
		if expected == actual {
			return
		}
	} else if exp, ok := expected.([]byte); ok {
		act, ok := actual.([]byte)
		if ok {
			if exp == nil && act == nil {
				return
			}
			if bytes.Equal(exp, act) {
				return
			}
		}
	} else if reflect.DeepEqual(expected, actual) {
		return
	}
	debug.PrintStack()
	t.Fatal("Not equal!", actual, expected)
}

func BenchmarkDictSet(b *testing.B) {
	undoTx := setup()
	b.ResetTimer()

	for i := 0; i < b.N; i++ {
		d.set(undoTx, []byte(strconv.Itoa(i%10000)), i)
	}
}

func BenchmarkMapInsert(b *testing.B) {
	mbench := make(map[string]int)
	b.ResetTimer()

	for i := 0; i < b.N; i++ {
		mbench[strconv.Itoa(i%10000)] = i
	}
}
