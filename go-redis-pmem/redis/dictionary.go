///////////////////////////////////////////////////////////////////////
// Copyright 2018-2019 VMware, Inc.
// SPDX-License-Identifier: BSD-2-Clause
///////////////////////////////////////////////////////////////////////

package redis

import (
	"bytes"
	"fmt"
	"hash"
	"hash/fnv"
	"math"
	"math/rand"
	"runtime"
	"sort"
	"strconv"
	"sync"
	"time"
	"unsafe"

	"github.com/vmware/go-pmem-transaction/transaction"
)

const (
	Ratio = 2
)

var (
	fnvHash hash.Hash32 = fnv.New32a()
)

type (
	dict struct {
		lock *sync.RWMutex
		tab  [2]table

		rehashLock *sync.RWMutex
		rehashIdx  int

		initSize       int
		bucketPerShard int
	}

	table struct {
		bucketlock []sync.RWMutex // finegrained bucket locks
		bucket     []*entry
		used       []int
		mask       int
	}

	entry struct {
		key   []byte
		value interface{}
		next  *entry
	}

	dictIterator struct {
		d                *dict
		index            int
		table, safe      int
		entry, nextentry *entry
		fingerprint      int64
	}
)

func NewDict(initSize, bucketPerShard int) *dict {
	d := pnew(dict)
	txn("undo") {
	d.initSize = nextPower(1, initSize)
	// TODO: add -1 value to indicate ALWAYS set bucketPerShard to dict size.
	if bucketPerShard >= initSize || bucketPerShard <= 0 {
		d.bucketPerShard = d.initSize
	} else if bucketPerShard > 0 {
		d.bucketPerShard = bucketPerShard
	}
	d.lock = new(sync.RWMutex)
	d.rehashLock = new(sync.RWMutex)
	d.resetTable(0, d.initSize)
	d.resetTable(1, 0)
	d.rehashIdx = -1
	}
	return d
}

func (d *dict) resetTable(i int, s int) {
	txn("undo") {
	if s == 0 {
		d.tab[i].bucketlock = nil
		d.tab[i].bucket = nil
		d.tab[i].used = nil
	} else {
		shards := d.shard(s)
		d.tab[i].bucketlock = make([]sync.RWMutex, shards)
		d.tab[i].bucket = pmake([]*entry, s)
		d.tab[i].used = pmake([]int, shards)
	}
	d.tab[i].mask = s - 1
	}
}

func inPMem(a unsafe.Pointer) {
	if a == nil {
		return
	}
	if !runtime.InPmem(uintptr(a)) {
		panic("Address not in pmem")
	}
}

func (d *dict) swizzle() {
	inPMem(unsafe.Pointer(d))
	d.lock = new(sync.RWMutex)
	d.rehashLock = new(sync.RWMutex)
	d.tab[0].swizzle(d)
	d.tab[1].swizzle(d)
}

func (t *table) swizzle(d *dict) {
	s := t.mask + 1
	if s > 0 {
		shards := d.shard(s)
		t.bucketlock = make([]sync.RWMutex, shards)
		inPMem(unsafe.Pointer(&t.bucket[0]))
		inPMem(unsafe.Pointer(&t.used[0]))
		total := 0
		x := 0
		for _, u := range t.used {
			total += u
		}
		// fmt.Println("Total kv pairs:", total)
		for _, e := range t.bucket {
			for e != nil {
				x++
				//println(x, e, e.key, e.value, e.next)
				e.swizzle()
				e = e.next
			}
		}
	}
}

func (e *entry) swizzle() {
	inPMem(unsafe.Pointer(e))
	if len(e.key) > 0 {
		inPMem(unsafe.Pointer(&(e.key[0])))
	}
	inPMem(unsafe.Pointer(e.next))
	//var tmp uintptr
	//word := uintptr(unsafe.Pointer(&e.value)) + uintptr(unsafe.Sizeof(tmp))
	//value := (**[]byte)(unsafe.Pointer(word))
	//inPMem(unsafe.Pointer(*value))
	switch v := e.value.(type) {
	case *[]byte:
		if len(*v) > 0 {
			inPMem(unsafe.Pointer(&(*v)[0]))
		}
	case *dict:
		v.swizzle()
	case *zset:
		v.swizzle()
	case int64:
	case float64:
	case nil:
	default:
		fmt.Printf("%T\n", e.value)
		panic("unknown type!")
	}
}

// get the shard number of a bucket id
func (d *dict) shard(b int) int {
	return b / d.bucketPerShard
}

func (d *dict) hashKey(key []byte) int {
	return dumbhash(key)
}

func fnvhash(key []byte) int {
	fnvHash.Write(key)
	h := int(fnvHash.Sum32())
	fnvHash.Reset()
	return h
}

func dumbhash(key []byte) int {
	h := 0
	for i, n := range key {
		h += int(math.Pow10(len(key)-i-1)) * int(n-'0')
	}
	return h
}

func memtierhash(key []byte) int {
	h, _ := strconv.Atoi(string(key[8:]))
	return h
}

// rehash and resize
func (d *dict) Cron(sleep time.Duration) {
	var used, size0, size1 int
	for {
		if size1 == 0 {
			time.Sleep(sleep) // reduce cpu consumption and lock contention
		}
		txn("undo") {
		d.rehashLock.Lock()
		if d.rehashIdx == -1 {
			// check whether need to resize table when rehashIdx < 0
			d.lock.Lock()
			used, size0, size1 = d.resizeIfNeeded()
			if size1 > 0 {
				fmt.Println("Dictionary used / size:", used, "/", size0,
					" ,Resize table to: ", size1)
			}
		} else if d.rehashIdx == -2 {
			d.lock.Lock()
			d.rehashSwap()
			size1 = 0
		} else {
			d.lock.RLock()
			d.rehashStep()
		}
		}
	}
}

func (d *dict) rehashStep() {
	if d.rehashIdx >= 0 && d.rehashIdx < len(d.tab[0].bucket) {
		d.lockShard(0, d.shard(d.rehashIdx))
		e := d.tab[0].bucket[d.rehashIdx]
		if e == nil {
			txn("undo") {
			d.rehashIdx++
			}
		} else {
			i0 := d.rehashIdx
			i1 := d.hashKey(e.key) & (d.tab[1].mask)
			s0 := d.shard(i0)
			s1 := d.shard(i1)
			txn("undo") {
			d.lockShard(1, s1)
			next := e.next
			e.next = d.tab[1].bucket[i1]
			d.tab[0].bucket[i0] = next
			d.tab[0].used[s0]--
			d.tab[1].bucket[i1] = e
			d.tab[1].used[s1]++
			}
		}
	}
	if d.rehashIdx == len(d.tab[0].bucket) {
		d.rehashIdx = -2
	}
}

func (d *dict) rehashSwap() {
	txn("undo") {
	d.tab[0] = d.tab[1]
	d.resetTable(1, 0)
	d.rehashIdx = -1
	}
	//fmt.Println("Rehash finished!")
}

func (d *dict) resizeIfNeeded() (used, size0, size1 int) {
	size0 = len(d.tab[0].bucket)
	used = d.size()

	if used > size0 {
		return used, size0, d.resize(used)
	} else if size0 > d.initSize && used < size0/Ratio {
		return used, size0, d.resize(used)
	} else {
		return used, size0, 0
	}
}

func (d *dict) resize(s int) int {
	s = nextPower(d.initSize, s)
	txn("undo") {
	d.resetTable(1, s)
	d.rehashIdx = 0
	}
	return s
}

func nextPower(s1, s2 int) int {
	if s1 < 1 {
		s1 = 1
	}
	for s1 < s2 {
		s1 *= 2
	}
	return s1
}

func (d *dict) lockKey(key []byte) {
	txn("undo") {
	d.lock.RLock()
	maxt := 0
	if d.tab[1].mask > 0 {
		maxt = 1
	}

	for t := 0; t <= maxt; t++ {
		s := d.findShard(t, key)
		d.lockShard(t, s)
	}
	}
}

func (d *dict) lockKeys(keys [][]byte, stride int) {
	txn("undo") {
	d.lock.RLock()

	maxt := 0
	if d.tab[1].mask > 0 {
		maxt = 1
	}
	shards := make([]int, len(keys)/stride)

	for t := 0; t <= maxt; t++ {
		for i, _ := range shards {
			shards[i] = d.findShard(t, keys[i*stride])
		}
		// make sure locks are acquired in the same order (ascending table and
		// bucket id) to prevent deadlock!
		sort.Ints(shards)
		prev := -1
		for _, s := range shards {
			// only lock distinct shards
			if s != prev {
				d.lockShard(t, s)
				prev = s
			}
		}
	}
	}
}

func (d *dict) lockAllKeys() {
	txn("undo") {
	d.lock.RLock()

	maxt := 0
	if d.tab[1].mask > 0 {
		maxt = 1
	}

	for t := 0; t <= maxt; t++ {
		for s := 0; s < d.shard(d.tab[t].mask+1); s++ {
			d.lockShard(t, s)
		}
	}
	}
}

func (d *dict) findShard(t int, key []byte) int {
	return d.shard(d.hashKey(key) & d.tab[t].mask)
}

func (d *dict) lockShard(t, s int) {
	// ReadOnly commands will aquire readOnly tx and read locks, otherwise
	// WLock is aquired.
	txn("undo") {
		d.tab[t].bucketlock[s].Lock()
	}
}

func (d *dict) find(key []byte) (int, int, *entry, *entry) {
	h := d.hashKey(key)
	var (
		maxt, b   int
		pre, curr *entry
	)
	if d.tab[1].mask > 0 {
		maxt = 1
	} else {
		maxt = 0
	}
	// fmt.Println("finding ", key)
	for i := 0; i <= maxt; i++ {
		b = h & d.tab[i].mask
		pre = nil
		curr = d.tab[i].bucket[b]
		for curr != nil {
			// fmt.Println("comparing with ", curr)
			if bytes.Compare(curr.key, key) == 0 {
				return i, b, pre, curr
			}
			pre = curr
			curr = curr.next
		}
	}
	return maxt, b, pre, curr
}

// key/value should be in pmem area, and appropriate locks should be already
// aquired at command level.
func (d *dict) set(key []byte, value interface{}) (insert bool) {
	t, b, _, e := d.find(key)

	if e != nil {
		txn("undo") {
		e.value = value
		}
		return false
	} else {
		e2 := pnew(entry)
		e2.key = key
		e2.value = value
		e2.next = d.tab[t].bucket[b]
		runtime.FlushRange(unsafe.Pointer(e2), unsafe.Sizeof(*e2)) // shadow update
		txn("undo") {
		d.tab[t].bucket[b] = e2
		s := d.shard(b)
		d.tab[t].used[s]++
		// fmt.Println("set entry: ", e2)
		}
		return true
	}
}

func (d *dict) delete(key []byte) *entry {
	t, b, p, e := d.find(key)
	txn("undo") {
	if e != nil { // note that gc should not recycle e before commit.
		if p != nil {
			p.next = e.next
		} else {
			d.tab[t].bucket[b] = e.next
		}

		s := d.shard(b)
		d.tab[t].used[s]--
	}
	}
	return e
}

func (d *dict) size() int {
	s := 0
	for _, t := range d.tab {
		s += t.size()
	}
	return s
}

func (t *table) size() int {
	s := 0
	if t.used != nil {
		for _, u := range t.used {
			s += u
		}
	}
	return s
}

func (d *dict) empty() {
	txn("undo") {
	d.resetTable(0, d.initSize)
	d.resetTable(1, 0)
	d.rehashIdx = -1
	}
}

func (d *dict) randomKey() *entry {
	if d.size() == 0 {
		return nil
	}

	// search from possible buckets
	var e *entry = nil
	if d.rehashIdx >= 0 { // rehashing
		for e == nil {
			h := d.rehashIdx + (rand.Int() % (d.tab[0].mask + d.tab[1].mask + 2 - d.rehashIdx))
			if h > d.tab[0].mask {
				e = d.tab[1].bucket[h-d.tab[0].mask-1]
			} else {
				e = d.tab[0].bucket[h]
			}
		}
	} else if d.rehashIdx == -1 { // not rehashing
		for e == nil {
			h := rand.Int() & d.tab[0].mask
			e = d.tab[0].bucket[h]
		}
	} else { // rehash finished but not swapping table
		for e == nil {
			h := rand.Int() & d.tab[1].mask
			e = d.tab[1].bucket[h]
		}
	}

	// found a non empty bucket, search a random one from the entry list
	ll := 0
	ee := e
	for ee != nil {
		ee = ee.next
		ll++
	}
	for i := 0; i < rand.Int()%ll; i++ {
		e = e.next
	}
	return e
}

func (d *dict) getIterator() *dictIterator {
	iter := &dictIterator{
		d:         d,
		table:     0,
		index:     -1,
		safe:      0,
		entry:     nil,
		nextentry: nil}
	return iter
}

func (i *dictIterator) next() *entry {
	for {
		if i.entry == nil {
			ht := i.d.tab[i.table]
			// TODO: implement safe iterator and fingerprint
			i.index++
			if i.index >= len(ht.bucket) {
				if i.table == 0 && len(i.d.tab[1].bucket) > 0 {
					i.table++
					i.index = 0
					ht = i.d.tab[1]
				} else {
					break
				}
			}
			i.entry = ht.bucket[i.index]
		} else {
			i.entry = i.nextentry
		}
		if i.entry != nil {
			i.nextentry = i.entry.next
			return i.entry
		}
	}
	return nil
}
