///////////////////////////////////////////////////////////////////////
// Copyright 2018-2019 VMware, Inc.
// SPDX-License-Identifier: BSD-2-Clause
///////////////////////////////////////////////////////////////////////

package redis

import (
	"fmt"

	"github.com/vmware/go-pmem-transaction/transaction"
)

type (
	quicklist struct {
		head, tail     *quicklistNode
		count, length  int
		fill, compress int
	}

	quicklistNode struct {
		prev, next *quicklistNode
		zl         *ziplist
	}

	quicklistIter struct {
		ql         *quicklist
		current    *quicklistNode
		offset, zi int
		startHead  bool
	}

	quicklistEntry struct {
		ql         *quicklist
		node       *quicklistNode
		value      interface{}
		offset, zi int
	}
)

var (
	optimization_level [5]int = [...]int{4096, 8192, 16384, 32768, 65536}
)

func quicklistCreate() *quicklist {
	ql := pnew(quicklist)
	txn("undo") {
	ql.fill = -2
	}
	return ql
}

func quicklistNew(fill, compress int) *quicklist {
	ql := quicklistCreate()
	ql.SetOptions(fill, compress)
	return ql
}

func quicklistCreateFromZiplist(fill, compress int, zl *ziplist) *quicklist {
	ql := quicklistNew(fill, compress)
	ql.AppendValuesFromZiplist(zl)
	return ql
}

func quicklistCreateNode() *quicklistNode {
	node := pnew(quicklistNode)
	txn("undo") {
	node.zl = ziplistNew()
	}
	return node
}

func (ql *quicklist) SetOptions(fill, compress int) {
	ql.SetFill(fill)
	ql.SetCompressDepth(compress)
}

func (ql *quicklist) SetFill( fill int) {
	if fill > 1<<15 {
		fill = 1 << 15
	} else if fill < -5 {
		fill = -5
	}
	txn("undo") {
	ql.fill = fill
	}
}

func (ql *quicklist) SetCompressDepth(compress int) {
	// TODO
}

func (ql *quicklist) Compress(node *quicklistNode) {
	// TODO
}

func (ql *quicklist) AppendValuesFromZiplist(zl *ziplist) {
	pos := 0
	val := zl.Get(pos)
	for val != nil {
		ql.PushTail(val)
		pos = zl.Next(pos)
		val = zl.Get(pos)
	}
}

func (ql *quicklist) Count() int {
	return ql.count
}

func (qe *quicklistEntry) Compare(val interface{}) bool {
	return qe.node.zl.Compare(qe.zi, val)
}

func (ql *quicklist) GetIterator(startHead bool) *quicklistIter {
	iter := new(quicklistIter)
	if startHead {
		iter.current = ql.head
		iter.offset = 0
	} else {
		iter.current = ql.tail
		iter.offset = -1
	}
	iter.startHead = startHead
	iter.ql = ql
	iter.zi = -1

	return iter
}

func (ql *quicklist) GetIteratorAtIdx(startHead bool, idx int) *quicklistIter {
	var entry quicklistEntry
	if ql.Index(idx, &entry) {
		base := ql.GetIterator(startHead)
		base.current = entry.node
		base.offset = entry.offset
		return base
	} else {
		return nil
	}
}

func (iter *quicklistIter) Next(entry *quicklistEntry) bool {
	if iter == nil {
		return false
	}
	entry.ql = iter.ql
	entry.node = iter.current
	if iter.current == nil {
		return false
	}

	if iter.zi < 0 {
		iter.zi = iter.current.zl.Index(iter.offset)
	} else {
		if iter.startHead {
			iter.zi = iter.current.zl.Next(iter.zi)
			iter.offset++
		} else {
			iter.zi = iter.current.zl.Prev(iter.zi)
			iter.offset--
		}
	}

	entry.zi = iter.zi
	entry.offset = iter.offset

	if iter.zi >= 0 {
		entry.value = entry.node.zl.Get(entry.zi)
		return true
	} else {
		if iter.startHead {
			iter.current = iter.current.next
			iter.offset = 0
		} else {
			iter.current = iter.current.prev
			iter.offset = -1
		}
		return iter.Next(entry)
	}
}

func (iter *quicklistIter) DelEntry(entry *quicklistEntry) {
	prev := entry.node.prev
	next := entry.node.next
	deleteNode := entry.ql.delIndex(entry.node, entry.zi)
	iter.zi = -1
	if deleteNode {
		if iter.startHead {
			iter.current = next
			iter.offset = 0
		} else {
			iter.current = prev
			iter.offset = -1
		}
	}
}

func (ql *quicklist) Index(idx int, entry *quicklistEntry) bool {
	n := new(quicklistNode)
	forward := (idx >= 0)
	var index, accum int
	if forward {
		index = idx
		n = ql.head
	} else {
		index = -idx - 1
		n = ql.tail
	}

	for n != nil {
		if accum+int(n.zl.entries) > index {
			break
		} else {
			accum += int(n.zl.entries)
			if forward {
				n = n.next
			} else {
				n = n.prev
			}
		}
	}

	if n == nil {
		return false
	}

	entry.node = n
	if forward {
		entry.offset = index - accum
	} else {
		entry.offset = -index - 1 + accum
	}

	entry.zi = n.zl.Index(entry.offset)
	entry.value = n.zl.Get(entry.zi)
	return true
}

func (ql *quicklist) Push(val interface{}, head bool) {
	if head {
		ql.PushHead(val)
	} else {
		ql.PushTail(val)
	}
}

func (ql *quicklist) PushHead(val interface{}) bool {
	origHead := ql.head
	txn("undo") {
	if origHead.allowInsert(ql.fill, val) {
		origHead.zl.Push(val, true)
	} else {
		node := quicklistCreateNode()
		node.zl.Push(val, true)
		ql.insertNodeBefore(ql.head, node)
	}
	ql.count++
	}
	return origHead != ql.head
}

func (ql *quicklist) PushTail(val interface{}) bool {
	origTail := ql.tail
	txn("undo") {
	if origTail.allowInsert(ql.fill, val) {
		origTail.zl.Push(val, false)
	} else {
		node := quicklistCreateNode()
		node.zl.Push(val, false)
		ql.insertNodeAfter(ql.tail, node)
	}
	ql.count++
	}
	return origTail != ql.tail
}

func (ql *quicklist) InsertBefore(entry *quicklistEntry, val interface{}) {
	ql.insert(entry, val, false)
}

func (ql *quicklist) InsertAfter(entry *quicklistEntry, val interface{}) {
	ql.insert(entry, val, true)
}

func (ql *quicklist) Pop(head bool) interface{} {
	if ql.count == 0 {
		return nil
	}
	var node *quicklistNode
	var idx int
	if head && ql.head != nil {
		node = ql.head
		idx = 0
	} else if !head && ql.tail != nil {
		node = ql.tail
		idx = -1
	} else {
		return nil
	}

	pos := node.zl.Index(idx)
	val := node.zl.Get(pos)
	if val != nil {
		ql.delIndex(node, pos)
	}
	return val
}

func (ql *quicklist) ReplaceAtIndex(idx int, val interface{}) bool {
	var entry quicklistEntry
	if ql.Index(idx, &entry) {
		entry.node.zl.Delete(entry.zi)
		entry.node.zl.insert(entry.zi, val)
		return true
	} else {
		return false
	}
}

func (ql *quicklist) DelRange(start, count int) int {
	if count <= 0 || start > ql.count || start < -ql.count {
		return 0
	}
	extent := count
	if start >= 0 && extent > ql.count-start {
		extent = ql.count - start
	} else if start < 0 && extent > -start {
		extent = -start
	}

	var entry quicklistEntry
	if !ql.Index(start, &entry) {
		return 0
	}
	node := entry.node
	toDel := extent
	txn("undo") {
	for extent > 0 {
		next := node.next
		del := 0
		deleteNode := false
		nodeCount := int(node.zl.entries)
		if entry.offset == 0 && extent >= nodeCount {
			deleteNode = true
			del = nodeCount
		} else if entry.offset >= 0 && extent >= nodeCount {
			del = nodeCount - entry.offset
		} else if entry.offset < 0 {
			del = -entry.offset
			if del > extent {
				del = extent
			}
		} else {
			del = extent
		}

		if deleteNode {
			ql.delNode(node)
		} else {
			node.zl.DeleteRange(entry.offset, uint(del))
			ql.count -= del
			if node.zl.Len() == 0 {
				ql.delNode(node)
			}
		}
		extent -= del
		node = next
		entry.offset = 0
	}
	}
	return toDel
}

func (ql *quicklist) insertNodeBefore(oldNode, newNode *quicklistNode) {
	ql.insertNode(oldNode, newNode, false)
}

func (ql *quicklist) insertNodeAfter(oldNode, newNode *quicklistNode) {
	ql.insertNode(oldNode, newNode, true)
}

// ql should be logged outside this call.
func (ql *quicklist) insertNode(oldNode, newNode *quicklistNode, after bool) {
	txn("undo") {
	if after {
		newNode.prev = oldNode
		if oldNode != nil {
			newNode.next = oldNode.next
			if oldNode.next != nil {
				oldNode.next.prev = newNode
			}
			oldNode.next = newNode
		}
		if ql.tail == oldNode {
			ql.tail = newNode
		}
	} else {
		newNode.next = oldNode
		if oldNode != nil {
			newNode.prev = oldNode.prev
			if oldNode.prev != nil {
				oldNode.prev.next = newNode
			}
			oldNode.prev = newNode
		}
		if ql.head == oldNode {
			ql.head = newNode
		}
	}
	if ql.length == 0 {
		ql.head = newNode
		ql.tail = newNode
	}
	if oldNode != nil {
		ql.Compress(oldNode)
	}
	ql.length++
	}
}

func (ql *quicklist) insert(entry *quicklistEntry, val interface{}, after bool) {
	node := entry.node
	if node == nil {
		txn("undo") {
		newNode := quicklistCreateNode()
		newNode.zl.Push(val, true)
		ql.insertNode(nil, newNode, false)
		ql.count++
		}
		return
	}
	full := false
	atTail := false
	atHead := false
	fullNext := false
	fullPrev := false
	txn("undo") {
	if !node.allowInsert(ql.fill, val) {
		full = true
	}
	if after && entry.offset == int(node.zl.entries) {
		atTail = true
		if !node.next.allowInsert(ql.fill, val) {
			fullNext = true
		}
	}
	if !after && entry.offset == 0 {
		atHead = true
		if !node.prev.allowInsert(ql.fill, val) {
			fullPrev = true
		}
	}

	if !full && after {
		// insert/append to current node after entry
		next := node.zl.Next(entry.zi)
		if next == -1 {
			node.zl.Push(val, false)
		} else {
			node.zl.insert(next, val)
		}
	} else if !full && !after {
		// insert to current node before entry
		node.zl.insert(entry.zi, val)
	} else if full && atTail && node.next != nil && !fullNext {
		// insert to head of next node
		newNode := node.next
		newNode.zl.Push(val, true)
	} else if full && atHead && node.prev != nil && !fullPrev {
		// append to tail of prev node
		newNode := node.prev
		newNode.zl.Push(val, false)
	} else if full && ((atTail && node.next != nil && fullNext) || (atHead && node.prev != nil && fullPrev)) {
		// create a new node
		newNode := quicklistCreateNode()
		newNode.zl = ziplistNew()
		newNode.zl.Push(val, false)
		ql.insertNode(node, newNode, after)
	} else if full {
		// need to split full node
		newNode := node.split(entry.offset, after)
		if after {
			newNode.zl.Push(val, true)
		} else {
			newNode.zl.Push(val, false)
		}
		ql.insertNode(node, newNode, after)
		ql.mergeNodes(node)
	}
	ql.count++
	}
}

func (node *quicklistNode) allowInsert(fill int, val interface{}) bool {
	if node == nil {
		return false
	}
	zlOverhead := 0
	size := 4 // approximate size for intger store
	s, ok := val.([]byte)
	if ok {
		size = len(s)
	}
	if size < 254 {
		zlOverhead = 1
	} else {
		zlOverhead = 5
	}

	if size < 64 {
		zlOverhead += 1
	} else if size < 16384 {
		zlOverhead += 2
	} else {
		zlOverhead += 5
	}

	newsize := node.zl.Len() + zlOverhead + size
	if nodeSizeMeetOptimizationRequirement(newsize, fill) {
		return true
	} else if newsize > 8192 {
		return false
	} else if int(node.zl.entries) < fill {
		return true
	} else {
		return false
	}
}

func (node *quicklistNode) split(offset int, after bool) *quicklistNode {
	newNode := quicklistCreateNode()
	// copy the whole zl into new node
	txn("undo") {
	newNode.zl = node.zl.deepCopy()

	// remove dup entries in two nodes
	if after {
		node.zl.DeleteRange(offset+1, node.zl.entries)
		newNode.zl.DeleteRange(0, uint(offset)+1)
	} else {
		node.zl.DeleteRange(0, uint(offset))
		newNode.zl.DeleteRange(offset, newNode.zl.entries)
	}
	}
	return newNode
}

// ql should be logged outside the call.
func (ql *quicklist) mergeNodes(center *quicklistNode) {
	fill := ql.fill
	var prev, prevPrev, next, nextNext, target *quicklistNode
	if center.prev != nil {
		prev = center.prev
		if center.prev.prev != nil {
			prevPrev = center.prev.prev
		}
	}

	if center.next != nil {
		next = center.next
		if center.next.next != nil {
			nextNext = center.next.next
		}
	}

	if ql.allowNodeMerge(prev, prevPrev, fill) {
		ql.ziplistMerge(prevPrev, prev)
	}

	if ql.allowNodeMerge(next, nextNext, fill) {
		ql.ziplistMerge(next, nextNext)
	}

	if ql.allowNodeMerge(center, center.prev, fill) {
		target = ql.ziplistMerge(center.prev, center)
	} else {
		target = center
	}

	if ql.allowNodeMerge(target, target.next, fill) {
		ql.ziplistMerge(target, target.next)
	}
}

func (ql *quicklist) allowNodeMerge(a, b *quicklistNode, fill int) bool {
	if a == nil || b == nil {
		return false
	}
	mergeSize := a.zl.Len() + b.zl.Len()
	if nodeSizeMeetOptimizationRequirement(mergeSize, fill) {
		return true
	} else if mergeSize > 8192 {
		return false
	} else if int(a.zl.entries+b.zl.entries) <= fill {
		return true
	} else {
		return false
	}
}

// ql should be logged outside the call.
func (ql *quicklist) ziplistMerge(a, b *quicklistNode) *quicklistNode {
	txn("undo") {
	a.zl.Merge(b.zl)
	b.zl.entries = 0
	ql.delNode(b)
	}
	return a
}

func nodeSizeMeetOptimizationRequirement(sz, fill int) bool {
	if fill >= 0 {
		return false
	}
	idx := -fill - 1
	if idx < len(optimization_level) {
		if sz <= optimization_level[idx] {
			return true
		} else {
			return false
		}
	} else {
		return false
	}
}

// ql should be logged outside the call.
func (ql *quicklist) delIndex(node *quicklistNode, pos int) bool {
	deleteNode := false
	txn("undo") {
	node.zl.Delete(pos)
	if node.zl.entries == 0 {
		ql.delNode(node)
		deleteNode = true
	}
	ql.count--
	}
	return deleteNode
}

// ql should be logged outside this call.
func (ql *quicklist) delNode(node *quicklistNode) {
	txn("undo") {
	if node.next != nil {
		node.next.prev = node.prev
	}
	if node.prev != nil {
		node.prev.next = node.next
	}

	if node == ql.tail {
		ql.tail = node.prev
	}
	if node == ql.head {
		ql.head = node.next
	}

	ql.Compress(nil)
	ql.count -= int(node.zl.entries)
	ql.length--
	}
}

func (ql *quicklist) print() {
	iter := ql.GetIterator(true)
	fmt.Print("(", ql.count, " ", ql.length, ")")
	var entry quicklistEntry
	var prev *quicklistNode
	for iter.Next(&entry) {
		if entry.node != prev {
			if prev != nil {
				fmt.Print("] ")
			}
			prev = entry.node
			fmt.Print("[")
		}
		switch s := entry.value.(type) {
		case []byte:
			fmt.Print(entry.offset, " ", string(s), ", ")
		default:
			fmt.Print(entry.offset, " ", s, ", ")
		}
	}
	fmt.Print("]\n")
}

func (ql *quicklist) verify() bool {
	size := 0
	iter := ql.GetIterator(true)
	var entry quicklistEntry
	for iter.Next(&entry) {
		size++
	}
	if size != ql.count {
		println("ql len mismatch when iterate forward!")
		return false
	}
	size = 0
	iter = ql.GetIterator(false)
	for iter.Next(&entry) {
		size++
	}
	if size != ql.count {
		println("ql len mismatch when iterate backward!")
		return false
	}
	size = 0
	node := ql.head
	for node != nil {
		size += int(node.zl.entries)
		if !node.zl.verify() {
			return false
		}
		node = node.next
	}
	if size != ql.count {
		println("ql len mismatch with zl entries!")
		return false
	}
	return true
}
