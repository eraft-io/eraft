///////////////////////////////////////////////////////////////////////
// Copyright 2018-2019 VMware, Inc.
// SPDX-License-Identifier: BSD-2-Clause
///////////////////////////////////////////////////////////////////////

package redis

import (
	"bytes"
)

type (
	listTypeIterator struct {
		subject interface{}
		forward bool
		iter    *quicklistIter
	}

	listTypeEntry struct {
		li    *listTypeIterator
		entry quicklistEntry
	}
)

// ============== list type commands ====================
func pushGenericCommand(c *client, head bool) {
	pushed := 0
	c.db.lockKeyWrite(c.argv[1])
	if o, ok := c.getListOrReply(c.db.lookupKeyWrite(c.argv[1]), nil); ok {
		for j := 2; j < c.argc; j++ {
			if o == nil {
				o = quicklistCreate() // TODO: set list option
				c.db.setKey(shadowCopyToPmem(c.argv[1]), o)
			}
			listTypePush(o, c.argv[j], head)
			pushed++
		}
		c.addReplyLongLong(listTypeLength(o))
	}
}

func lpushCommand(c *client) {
	pushGenericCommand(c, true)
}

func rpushCommand(c *client) {
	pushGenericCommand(c, false)
}

func pushxGenericCommand(c *client, head bool) {
	c.db.lockKeyWrite(c.argv[1])
	o, ok := c.getListOrReply(c.db.lookupKeyWrite(c.argv[1]), shared.czero)
	if !ok || o == nil {
		return
	}
	pushed := 0
	for j := 2; j < c.argc; j++ {
		listTypePush(o, c.argv[j], head)
		pushed++
	}
	c.addReplyLongLong(listTypeLength(o))
}

func lpushxCommand(c *client) {
	pushxGenericCommand(c, true)
}

func rpushxCommand(c *client) {
	pushxGenericCommand(c, false)
}

func linsertCommand(c *client) {
	after := false
	if bytes.EqualFold(c.argv[2], []byte("after")) {
		after = true
	} else if bytes.EqualFold(c.argv[2], []byte("before")) {
		after = false
	} else {
		c.addReply(shared.syntaxerr)
		return
	}

	c.db.lockKeyWrite(c.argv[1])
	o, ok := c.getListOrReply(c.db.lookupKeyWrite(c.argv[1]), shared.czero)
	if !ok || o == nil {
		return
	}

	var entry listTypeEntry
	inserted := false
	iter := listTypeInitIterator(o, 0, true)
	for listTypeNext(iter, &entry) {
		if listTypeEqual(&entry, c.argv[3]) {
			listTypeInsert(&entry, c.argv[4], after)
			inserted = true
			break
		}
	}
	if !inserted {
		c.addReply(shared.cnegone)
	} else {
		c.addReplyLongLong(listTypeLength(o))
	}
}

func llenCommand(c *client) {
	if c.db.lockKeyRead(c.argv[1]) {
		if o, ok := c.getListOrReply(c.db.lookupKeyRead(c.argv[1]), nil); ok {
			c.addReplyLongLong(listTypeLength(o))
		}
	} else {
		c.addReply(shared.czero)
	}
}

func lindexCommand(c *client) {
	if c.db.lockKeyRead(c.argv[1]) {
		o, ok := c.getListOrReply(c.db.lookupKeyRead(c.argv[1]), shared.nullbulk)
		if !ok || o == nil {
			return
		}
		index, ok := c.getLongLongOrReply(c.argv[2], nil)
		if !ok {
			return
		}
		switch l := o.(type) {
		case *quicklist:
			var entry quicklistEntry
			if l.Index(int(index), &entry) {
				s, _ := getString(entry.value)
				c.addReplyBulk(s)
			} else {
				c.addReply(shared.nullbulk)
			}
		default:
			panic("Unknown list encoding")
		}
	} else {
		c.addReply(shared.nullbulk)
	}
}

func lsetCommand(c *client) {
	c.db.lockKeyWrite(c.argv[1])
	o, ok := c.getListOrReply(c.db.lookupKeyWrite(c.argv[1]), shared.nokeyerr)
	if !ok || o == nil {
		return
	}
	index, ok := c.getLongLongOrReply(c.argv[2], nil)
	if !ok {
		return
	}
	switch ql := o.(type) {
	case *quicklist:
		replaced := ql.ReplaceAtIndex(int(index), c.argv[3])
		if !replaced {
			c.addReply(shared.outofrangeerr)
		} else {
			c.addReply(shared.ok)
		}
	default:
		panic("Unknown list encoding")
	}
}

func popGenericCommand(c *client, head bool) {
	c.db.lockKeyWrite(c.argv[1])
	o, ok := c.getListOrReply(c.db.lookupKeyWrite(c.argv[1]), shared.nullbulk)
	if !ok || o == nil {
		return
	}

	val := listTypePop(o, head)
	if val == nil {
		c.addReply(shared.nullbulk)
	} else {
		s, _ := getString(val)
		c.addReplyBulk(s)
		if listTypeLength(o) == 0 {
			c.db.delete(c.argv[1])
		}
	}
}

func lpopCommand(c *client) {
	popGenericCommand(c, true)
}

func rpopCommand(c *client) {
	popGenericCommand(c, false)
}

func lrangeCommand(c *client) {
	var start, end int64
	var ok bool
	if start, ok = c.getLongLongOrReply(c.argv[2], nil); !ok {
		return
	}
	if end, ok = c.getLongLongOrReply(c.argv[3], nil); !ok {
		return
	}
	if !c.db.lockKeyRead(c.argv[1]) {
		c.addReply(shared.emptymultibulk)
		return
	}
	o, ok := c.getListOrReply(c.db.lookupKeyRead(c.argv[1]), shared.emptymultibulk)
	if !ok || o == nil {
		return
	}
	llen := listTypeLength(o)

	// convert negative indexes
	if start < 0 {
		start += llen
	}
	if end < 0 {
		end += llen
	}
	if start < 0 {
		start = 0
	}
	if start > end || start >= llen {
		c.addReply(shared.emptymultibulk)
		return
	}
	if end >= llen {
		end = llen - 1
	}
	rangelen := int(end-start) + 1

	c.addReplyMultiBulkLen(rangelen)
	switch o.(type) {
	case *quicklist:
		iter := listTypeInitIterator(o, int(start), true)
		for ; rangelen > 0; rangelen-- {
			var entry listTypeEntry
			listTypeNext(iter, &entry)
			qe := &entry.entry
			s, _ := getString(qe.value)
			c.addReplyBulk(s)
		}
	default:
		panic("Unknown list encoding")
	}
}

func ltrimCommand(c *client) {
	var start, end, ltrim, rtrim int64
	var ok bool
	if start, ok = c.getLongLongOrReply(c.argv[2], nil); !ok {
		return
	}
	if end, ok = c.getLongLongOrReply(c.argv[3], nil); !ok {
		return
	}
	c.db.lockKeyWrite(c.argv[1])
	o, ok := c.getListOrReply(c.db.lookupKeyWrite(c.argv[1]), shared.ok)
	if !ok || o == nil {
		return
	}
	llen := listTypeLength(o)

	// convert negative indexes
	if start < 0 {
		start += llen
	}
	if end < 0 {
		end += llen
	}
	if start < 0 {
		start = 0
	}
	if start > end || start >= llen {
		ltrim = llen
		rtrim = 0
	} else {
		if end >= llen {
			end = llen - 1
		}
		ltrim = start
		rtrim = llen - end - 1
	}
	switch l := o.(type) {
	case *quicklist:
		l.DelRange(0, int(ltrim))
		l.DelRange(int(-rtrim), int(rtrim))
	default:
		panic("Unknown list encoding")
	}
	c.addReply(shared.ok)
}

func lremCommand(c *client) {
	var (
		toremove, removed int64
		ok                bool
		li                *listTypeIterator
		entry             listTypeEntry
	)

	if toremove, ok = c.getLongLongOrReply(c.argv[2], nil); !ok {
		return
	}
	c.db.lockKeyWrite(c.argv[1])
	o, ok := c.getListOrReply(c.db.lookupKeyWrite(c.argv[1]), shared.ok)
	if !ok || o == nil {
		return
	}
	if toremove < 0 {
		toremove = -toremove
		li = listTypeInitIterator(o, -1, false)
	} else {
		li = listTypeInitIterator(o, 0, true)
	}

	for listTypeNext(li, &entry) {
		if listTypeEqual(&entry, c.argv[3]) {
			listTypeDelete(li, &entry)
			removed++
			if toremove > 0 && toremove == removed {
				break
			}
		}
	}
	if listTypeLength(o) == 0 {
		c.db.delete(c.argv[1])
	}
	c.addReplyLongLong(removed)
}

func rpoplpushCommand(c *client) {
	c.db.lockKeysWrite(c.argv[1:3], 1)
	sobj, ok := c.getListOrReply(c.db.lookupKeyWrite(c.argv[1]), shared.nullbulk)
	if !ok || sobj == nil {
		return
	}
	if listTypeLength(sobj) == 0 {
		c.addReply(shared.nullbulk)
	} else {
		dobj, ok := c.getListOrReply(c.db.lookupKeyWrite(c.argv[2]), nil)
		if !ok {
			return
		}
		value := listTypePop(sobj, false)
		rpoplpushHandlePush(c, c.argv[2], dobj, value)
		if listTypeLength(sobj) == 0 {
			c.db.delete(c.argv[1])
		}
	}
}

// ============== helper functions ====================
func listTypePush(o, val interface{}, head bool) {
	switch l := o.(type) {
	case *quicklist:
		l.Push(val, head)
	default:
		panic("Unknown list encoding")
	}
}

func listTypeLength(o interface{}) int64 {
	switch l := o.(type) {
	case *quicklist:
		return int64(l.Count())
	case nil:
		return 0
	default:
		panic("Unknown list encoding")
	}
}

func listTypeInitIterator(o interface{}, index int, forward bool) *listTypeIterator {
	li := new(listTypeIterator)
	li.subject = o
	li.forward = forward
	if ql, ok := o.(*quicklist); ok {
		li.iter = ql.GetIteratorAtIdx(forward, index)
	} else {
		panic("Unknown list encoding")
	}
	return li
}

func listTypeNext(li *listTypeIterator, entry *listTypeEntry) bool {
	entry.li = li
	return li.iter.Next(&entry.entry)
}

func listTypeEqual(entry *listTypeEntry, val interface{}) bool {
	return entry.entry.Compare(val)
}

func listTypeInsert(entry *listTypeEntry, val interface{}, after bool) {
	if after {
		entry.entry.ql.InsertAfter(&entry.entry, val)
	} else {
		entry.entry.ql.InsertBefore(&entry.entry, val)
	}
}

func listTypePop(o interface{}, head bool) interface{} {
	switch l := o.(type) {
	case *quicklist:
		return l.Pop(head)
	default:
		panic("Unknown list encoding")
	}
}

func listTypeDelete(li *listTypeIterator, entry *listTypeEntry) {
	li.iter.DelEntry(&entry.entry)
}

func rpoplpushHandlePush(c *client, dstkey []byte, dstobj, value interface{}) {
	if dstobj == nil {
		dstobj = quicklistCreate()
		c.db.setKey(shadowCopyToPmem(dstkey), dstobj)
	}
	listTypePush(dstobj, value, true)
	s, _ := getString(value)
	c.addReplyBulk(s)
}
