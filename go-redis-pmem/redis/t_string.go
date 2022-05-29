///////////////////////////////////////////////////////////////////////
// Copyright 2018-2019 VMware, Inc.
// SPDX-License-Identifier: BSD-2-Clause
///////////////////////////////////////////////////////////////////////

package redis

import (
	"math"
	"strconv"
	"time"
	"github.com/vmware/go-pmem-transaction/transaction"
)

const (
	OBJ_SET_NO_FLAGS = 0
	OBJ_SET_NX       = 1 << 0
	OBJ_SET_XX       = 1 << 1
	OBJ_SET_EX       = 1 << 2
	OBJ_SET_PX       = 1 << 3
)

// SET key value [NX] [XX] [EX <seconds>] [PX <milliseconds>]
func setCommand(c *client) {
	ms := false
	var expire []byte
	flags := OBJ_SET_NO_FLAGS

	for i := 3; i < c.argc; i++ {
		curr := c.argv[i]
		var next []byte
		if i < c.argc-1 {
			next = c.argv[i+1]
		}
		if (curr[0] == 'n' || curr[0] == 'N') &&
			(curr[1] == 'x' || curr[1] == 'X') &&
			len(curr) == 2 && (flags&OBJ_SET_XX) == 0 {
			flags |= OBJ_SET_NX
		} else if (curr[0] == 'x' || curr[0] == 'X') &&
			(curr[1] == 'x' || curr[1] == 'X') &&
			len(curr) == 2 && (flags&OBJ_SET_NX) == 0 {
			flags |= OBJ_SET_XX
		} else if (curr[0] == 'e' || curr[0] == 'E') &&
			(curr[1] == 'x' || curr[1] == 'X') &&
			len(curr) == 2 && (flags&OBJ_SET_PX) == 0 {
			flags |= OBJ_SET_EX
			expire = next
			i++
		} else if (curr[0] == 'p' || curr[0] == 'P') &&
			(curr[1] == 'x' || curr[1] == 'X') &&
			len(curr) == 2 && (flags&OBJ_SET_EX) == 0 {
			flags |= OBJ_SET_PX
			ms = true
			expire = next
			i++
		} else {
			c.addReply(shared.syntaxerr)
			return
		}
	}
	setGeneric(c, flags, c.argv[1], c.argv[2], expire, ms, nil, nil)
}

func setnxCommand(c *client) {
	setGeneric(c, OBJ_SET_NX, c.argv[1], c.argv[2], nil, false, shared.cone, shared.czero)
}

func setexCommand(c *client) {
	setGeneric(c, OBJ_SET_NO_FLAGS, c.argv[1], c.argv[3], c.argv[2], false, nil, nil)
}

func psetexCommand(c *client) {
	setGeneric(c, OBJ_SET_NO_FLAGS, c.argv[1], c.argv[3], c.argv[2], true, nil, nil)
}

func setGeneric(c *client, flags int, key, val []byte, expire []byte, ms bool, okReply, abortReply []byte) {
	var ns int64 = -1
	if expire != nil {
		t, ok := c.getLongLongOrReply(expire, nil)
		if !ok {
			return
		}
		if t <= 0 {
			c.addReplyError([]byte("invalid expire time"))
			return
		}
		if ms {
			ns = t * 1000000
		} else {
			ns = t * 1000000000
		}
	}

	c.db.lockKeyWrite(key)
	if ((flags&OBJ_SET_NX) > 0 && c.db.lookupKeyWrite(key) != nil) ||
		((flags&OBJ_SET_XX) > 0 && c.db.lookupKeyWrite(key) == nil) {
		if abortReply != nil {
			c.addReply(abortReply)
		} else {
			c.addReply(shared.nullbulk)
		}
		return
	}

	c.db.setKey(shadowCopyToPmem(key), shadowCopyToPmemI(val))
	if expire != nil {
		when := time.Now().UnixNano() + ns
		c.db.setExpire(key, when)
	}

	if okReply != nil {
		c.addReply(okReply)
	} else {
		c.addReply(shared.ok)
	}
}

func getCommand(c *client) {
	if c.db.lockKeyRead(c.argv[1]) {
		getGeneric(c)
	} else { // expired
		c.addReply(shared.nullbulk)
	}
}

func getsetCommand(c *client) {
	c.db.lockKeyWrite(c.argv[1])
	if !getGeneric(c) {
		return
	}
	c.db.setKey(shadowCopyToPmem(c.argv[1]), shadowCopyToPmemI(c.argv[2]))
}

func getGeneric(c *client) bool {
	i := c.db.lookupKeyRead(c.argv[1])
	v, ok := c.getStringOrReply(i, shared.nullbulk, shared.wrongtypeerr)
	if v != nil {
		c.addReplyBulk(v)
	}
	return ok
}

func setrangeCommand(c *client) {
	offset, err := slice2i(c.argv[2])
	if err != nil {
		c.addReplyError([]byte("value is not an integer"))
	}
	if offset < 0 {
		c.addReplyError([]byte("offset is out of range"))
		return
	}
	update := c.argv[3]
	c.db.lockKeyWrite(c.argv[1])
	v, ok := c.getStringOrReply(c.db.lookupKeyWrite(c.argv[1]), nil, shared.wrongtypeerr)
	if !ok {
		return
	}

	if len(update) == 0 { // no updates
		c.addReplyLongLong(int64(len(v)))
	} else {
		needed := offset + len(update)
		if !checkStringLength(c, needed) {
			return
		}
		if needed > len(v) {
			c.db.setKey(shadowCopyToPmem(c.argv[1]), shadowConcatToPmemI(v, update, offset, needed))
			c.addReplyLongLong(int64(needed))
		} else {
			txn("undo") {
			copy(v[offset:], update) // TODO: (mohitv) copy not supported yet
			c.addReplyLongLong(int64(len(v)))
			}
		}
	}
}

func getrangeCommand(c *client) {
	start, err := slice2i(c.argv[2])
	if err != nil {
		c.addReplyError([]byte("value is not an integer"))
	}
	end, err := slice2i(c.argv[3])
	if err != nil {
		c.addReplyError([]byte("value is not an integer"))
	}

	var v []byte
	var ok bool
	if c.db.lockKeyRead(c.argv[1]) { // not expired
		v, ok = c.getStringOrReply(c.db.lookupKeyRead(c.argv[1]), shared.emptybulk, shared.wrongtypeerr)
		if v == nil || !ok {
			return
		}
	}

	strlen := len(v)

	if start < 0 {
		start += strlen
	}
	if end < 0 {
		end += strlen
	}
	if start < 0 {
		start = 0
	}
	if end < 0 {
		end = 0
	}
	if end >= strlen {
		end = strlen - 1
	}

	if start > end || strlen == 0 {
		c.addReply(shared.emptybulk)
	} else {
		c.addReplyBulk(v[start : end+1])
	}
}

func mgetCommand(c *client) {
	c.addReplyMultiBulkLen(c.argc - 1)
	alives := c.db.lockKeysRead(c.argv[1:], 1)
	for i, k := range c.argv[1:] {
		if alives[i] {
			v, _ := c.getStringOrReply(c.db.lookupKeyRead(k), shared.nullbulk, shared.nullbulk)
			if v != nil {
				c.addReplyBulk(v)
			}
		} else { // expired key
			c.addReply(shared.nullbulk)
		}
	}
}

func msetCommand(c *client) {
	msetGeneric(c, false)
}

func msetnxCommand(c *client) {
	msetGeneric(c, true)
}

func msetGeneric(c *client, nx bool) {
	if c.argc%2 == 0 {
		c.addReplyError([]byte("wrong number of arguments for MSET"))
		return
	}
	c.db.lockKeysWrite(c.argv[1:], 2)
	if nx {
		for i := 1; i < c.argc; i += 2 {
			if c.db.lookupKeyWrite(c.argv[i]) != nil {
				c.addReply(shared.czero)
				return
			}
		}
	}

	for i := 1; i < c.argc; i += 2 {
		c.db.setKey(shadowCopyToPmem(c.argv[i]), shadowCopyToPmemI(c.argv[i+1]))
	}
	if nx {
		c.addReply(shared.cone)
	} else {
		c.addReply(shared.ok)
	}
}

func appendCommand(c *client) {
	totlen := 0
	c.db.lockKeyWrite(c.argv[1])
	v, ok := c.getStringOrReply(c.db.lookupKeyWrite(c.argv[1]), nil, shared.wrongtypeerr)
	if !ok {
		return
	}
	if v == nil {
		// Create the key
		c.db.setKey(shadowCopyToPmem(c.argv[1]), shadowCopyToPmemI(c.argv[2]))
		totlen = len(c.argv[2])
	} else {
		// Append the value
		totlen = len(v) + len(c.argv[2])
		if !checkStringLength(c, totlen) {
			return
		}
		// no need to copy c.argv[1] into pmem as we already know it exists in db in this case.
		c.db.setKey(c.argv[1], shadowConcatToPmemI(v, c.argv[2], len(v), totlen))
	}
	c.addReplyLongLong(int64(totlen))
}

func strlenCommand(c *client) {
	if c.db.lockKeyRead(c.argv[1]) {
		if v, _ := c.getStringOrReply(c.db.lookupKeyRead(c.argv[1]),
			shared.czero, shared.wrongtypeerr); v != nil {
			c.addReplyLongLong(int64(len(v)))
		}
	} else { // expired
		c.addReply(shared.czero)
	}
}

func checkStringLength(c *client, len int) bool {
	if len > 512*1024*1024 {
		c.addReplyError([]byte("string exceeds maximum allowed size (512MB)"))
		return false
	}
	return true
}

func incrCommand(c *client) {
	incrDecrCommand(c, 1)
}

func decrCommand(c *client) {
	incrDecrCommand(c, -1)
}

func incrbyCommand(c *client) {
	if incr, ok := c.getLongLongOrReply(c.argv[2], nil); ok {
		incrDecrCommand(c, incr)
	}
}

func decrbyCommand(c *client) {
	if incr, ok := c.getLongLongOrReply(c.argv[2], nil); ok {
		incrDecrCommand(c, -incr)
	}
}

func incrDecrCommand(c *client, incr int64) {
	c.db.lockKeyWrite(c.argv[1])
	if v, ok := c.getLongLongOrReply(c.db.lookupKeyWrite(c.argv[1]), nil); ok {
		if (incr < 0 && v < 0 && incr < (math.MinInt64-v)) ||
			(incr > 0 && v > 0 && incr > (math.MaxInt64-v)) {
			c.addReplyError([]byte("increment or decrement would overflow"))
			return
		}

		v += incr

		// TODO: use shared integers?
		c.db.setKey(shadowCopyToPmem(c.argv[1]), v)
		c.addReplyLongLong(v)
	}
}

func incrbyfloatCommand(c *client) {
	c.db.lockKeyWrite(c.argv[1])
	if v, ok := c.getLongDoubleOrReply(c.db.lookupKeyWrite(c.argv[1]), nil); ok {
		if incr, ok := c.getLongDoubleOrReply(c.argv[2], nil); ok {
			v += incr
			if math.IsNaN(v) || math.IsInf(v, 0) {
				c.addReplyError([]byte("increment would produce NaN or Infinity"))
				return
			}
			c.db.setKey(shadowCopyToPmem(c.argv[1]), v)
			c.addReplyBulk([]byte(strconv.FormatFloat(v, 'f', -1, 64)))
		}
	}
}
