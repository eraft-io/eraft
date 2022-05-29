///////////////////////////////////////////////////////////////////////
// Copyright 2018-2019 VMware, Inc.
// SPDX-License-Identifier: BSD-2-Clause
///////////////////////////////////////////////////////////////////////

package redis

import (
	"bytes"
	"math"
	"math/rand"
	"sort"
	"strings"
	"unsafe"

	"github.com/vmware/go-pmem-transaction/transaction"
)

type (
	zset struct {
		dict *dict
		zsl  *zskiplist
	}

	zskiplist struct {
		header, tail *zskiplistNode
		length       uint
		level        int
	}

	zskiplistNode struct {
		ele      []byte
		score    float64
		backward *zskiplistNode
		level    []zskiplistLevel
	}

	zskiplistLevel struct {
		forward *zskiplistNode
		span    uint
	}

	zrangespec struct {
		min, max     float64
		minex, maxex bool
	}

	zlexrangespec struct {
		min, max     []byte
		minex, maxex bool
	}

	zsetopsrc struct {
		subject interface{}
		weight  float64
		iter    interface{}
	}
	zsetops []zsetopsrc

	zsetIter struct {
		zs   *zset
		node *zskiplistNode
	}

	zsetopval struct {
		flags int
		ele   []byte
		score float64
	}
)

const (
	// Input flags.
	ZADD_NONE = 0
	ZADD_INCR = (1 << 0)
	ZADD_NX   = (1 << 1)
	ZADD_XX   = (1 << 2)

	// Output flags.
	ZADD_NOP     = (1 << 3)
	ZADD_NAN     = (1 << 4)
	ZADD_ADDED   = (1 << 5)
	ZADD_UPDATED = (1 << 6)

	// Flags only used by the ZADD command but not by zsetAdd() API:
	ZADD_CH = (1 << 16)

	ZRANGE_RANK  = 0
	ZRANGE_SCORE = 1
	ZRANGE_LEX   = 2

	ZSKIPLIST_MAXLEVEL = 32
	ZSKIPLIST_P        = 0.25

	REDIS_AGGR_SUM = 1
	REDIS_AGGR_MIN = 2
	REDIS_AGGR_MAX = 3

	SET_OP_UNION        = 0
	SET_OP_DIFF         = 1
	SET_OP_INTER        = 2
	SET_OP_UNION_NOLOCK = 3
)

func (zs *zset) swizzle() {
	inPMem(unsafe.Pointer(zs))
	zs.zsl.swizzle()
	zs.dict.swizzle()
}

func (zsl *zskiplist) swizzle() {
	inPMem(unsafe.Pointer(zsl))
	inPMem(unsafe.Pointer(zsl.header))
	inPMem(unsafe.Pointer(zsl.tail))
	length := uint(0)
	var b *zskiplistNode
	x := zsl.header.level[0].forward
	for x != nil {
		inPMem(unsafe.Pointer(x))
		if len(x.ele) > 0 {
			inPMem(unsafe.Pointer(&(x.ele[0])))
		}
		inPMem(unsafe.Pointer(x.backward))
		if x.backward != b {
			panic("skiplist backward does not match!")
		}
		for _, l := range x.level {
			inPMem(unsafe.Pointer(l.forward))
		}
		b = x
		x = x.level[0].forward
		length++
	}
	if length != zsl.length {
		print(length, zsl.length)
		panic("skiplist length does not match!")
	}
}

// ============== zset type commands ====================
func zaddCommand(c *client) {
	zaddGenericCommand(c, ZADD_NONE)
}

func zincrbyCommand(c *client) {
	zaddGenericCommand(c, ZADD_INCR)
}

func zaddGenericCommand(c *client, flags int) {
	nanerr := []byte("resulting score is not a number (NaN)")

	// Parse options. At the end 'scoreidx' is set to the argument position
	// of the score of the first score-element pair.
	scoreidx := 2
	for scoreidx < c.argc {
		opt := string(c.argv[scoreidx])
		if strings.EqualFold(opt, "nx") {
			flags |= ZADD_NX
		} else if strings.EqualFold(opt, "xx") {
			flags |= ZADD_XX
		} else if strings.EqualFold(opt, "ch") {
			flags |= ZADD_CH
		} else if strings.EqualFold(opt, "incr") {
			flags |= ZADD_INCR
		} else {
			break
		}
		scoreidx++
	}

	incr := (flags & ZADD_INCR) != 0
	nx := (flags & ZADD_NX) != 0
	xx := (flags & ZADD_XX) != 0
	ch := (flags & ZADD_CH) != 0

	// After the options, we expect to have an even number of args, since
	// we expect any number of score-element pairs.
	elements := c.argc - scoreidx
	if elements%2 == 1 || elements == 0 {
		c.addReply(shared.syntaxerr)
		return
	}
	elements /= 2

	// Check for incompatible options.
	if nx && xx {
		c.addReplyError([]byte("XX and NX options at the same time are not compatible"))
		return
	}

	if incr && elements > 1 {
		//c.addReplyError([]byte("INCR option supports a single increment-element pair"))
		// TODO: check arity for cmd before the command function
		c.addReplyError([]byte("wrong number of arguments"))
		return
	}

	var zobj interface{}
	var ok bool
	var score float64
	var added int64
	var updated int64
	processed := 0

	// Start parsing all the scores, we need to emit any syntax error
	// before executing additions to the sorted set, as the command should
	// either execute fully or nothing at all.
	scores := make([]float64, elements)
	for j := 0; j < elements; j++ {
		if scores[j], ok = c.getLongDoubleOrReply(c.argv[scoreidx+j*2], nil); !ok {
			goto cleanup
		}
	}

	// Lookup the key and create the sorted set if does not exist.
	c.db.lockKeyWrite(c.argv[1])
	zobj = c.db.lookupKeyWrite(c.argv[1])
	if zobj == nil {
		if xx {
			goto reply_to_client // No key + XX option: nothing to do.
		}
		// TODO: implement ziplist encoding.
		zobj = zsetCreate()
		c.db.setKey(shadowCopyToPmem(c.argv[1]), zobj)
	} else {
		switch zobj.(type) {
		case *zset:
			break
		default:
			c.addReplyError(shared.wrongtypeerr)
			goto cleanup
		}
	}

	for j := 0; j < elements; j++ {
		score = scores[j]

		ele := c.argv[scoreidx+1+j*2]
		retval, newscore, retflags := zsetAdd(c, zobj, score, ele, flags)
		if retval == 0 {
			c.addReplyError(nanerr)
			goto cleanup
		}
		if retflags&ZADD_ADDED != 0 {
			added++
		}
		if retflags&ZADD_UPDATED != 0 {
			updated++
		}
		if retflags&ZADD_NOP == 0 {
			processed++
		}
		score = newscore
	}
	// server.dirty += (added+updated)

reply_to_client:
	if incr { // ZINCRBY or INCR option.
		if processed > 0 {
			c.addReplyDouble(score)
		} else {
			c.addReply(shared.nullbulk)
		}
	} else { // ZADD.
		if ch {
			c.addReplyLongLong(added + updated)
		} else {
			c.addReplyLongLong(added)
		}
	}

cleanup:
	if added > 0 || updated > 0 {
		// TODO: notify key space event
	}
}

func zremCommand(c *client) {
	c.db.lockKeyWrite(c.argv[1])
	zobj, ok := c.getZsetOrReply(c.db.lookupKeyRead(c.argv[1]), shared.emptymultibulk)
	if !ok || zobj == nil {
		return
	}

	var deleted int64
	keyremoved := false
	for j := 2; j < c.argc; j++ {
		if zsetDel(c, zobj, c.argv[j]) {
			deleted++
		}
		if zsetLength(zobj) == 0 {
			c.db.delete(c.argv[1])
			keyremoved = true
			break
		}
	}

	if deleted > 0 {
		// notify key space event
		if keyremoved {

		}
	}
	c.addReplyLongLong(deleted)
}

func zremrangebyrankCommand(c *client) {
	zremrangeGenericCommand(c, ZRANGE_RANK)
}

func zremrangebyscoreCommand(c *client) {
	zremrangeGenericCommand(c, ZRANGE_SCORE)
}

func zremrangebylexCommand(c *client) {
	zremrangeGenericCommand(c, ZRANGE_LEX)
}

func zremrangeGenericCommand(c *client, rangetype int) {
	var (
		start, end, llen int64
		deleted          uint
		zrange           zrangespec
		lexrange         zlexrangespec
		ok, keyremoved   bool
		key              []byte = c.argv[1]
	)

	// Step 1: Parse the range.
	if rangetype == ZRANGE_RANK {
		if start, ok = c.getLongLongOrReply(c.argv[2], nil); !ok {
			return
		}
		if end, ok = c.getLongLongOrReply(c.argv[3], nil); !ok {
			return
		}
	} else if rangetype == ZRANGE_SCORE {
		if zrange, ok = zslParseRange(c.argv[2], c.argv[3]); !ok {
			c.addReplyError([]byte("min or max is not a float"))
			return
		}
	} else if rangetype == ZRANGE_LEX {
		if lexrange, ok = zslParseLexRange(c.argv[2], c.argv[3]); !ok {
			c.addReplyError([]byte("min or max not valid string range item"))
			return
		}
	}

	// Step 2: Lookup & range sanity checks if needed.
	c.db.lockKeyWrite(key)
	zobj, ok := c.getZsetOrReply(c.db.lookupKeyWrite(key), shared.czero)
	if !ok || zobj == nil {
		return
	}

	if rangetype == ZRANGE_RANK {
		// Sanitize indexes.
		llen = int64(zsetLength(zobj))
		if start < 0 {
			start = start + llen
		}
		if end < 0 {
			end = end + llen
		}
		if start < 0 {
			start = 0
		}

		// Invariant: start >= 0, so this test will be true when end < 0.
		// The range is empty when start > end or start >= length.
		if start > end || start >= llen {
			c.addReply(shared.czero)
			return
		}
		if end >= llen {
			end = llen - 1
		}
	}

	// Step 3: Perform the range deletion operation.
	switch z := zobj.(type) {
	case *zset:
		if rangetype == ZRANGE_RANK {
			deleted = zslDeleteRangeByRank(z.zsl, uint(start+1), uint(end+1), z.dict)
		} else if rangetype == ZRANGE_SCORE {
			deleted = zslDeleteRangeByScore(z.zsl, &zrange, z.dict)
		} else {
			deleted = zslDeleteRangeByLex(z.zsl, &lexrange, z.dict)
		}
		if z.zsl.length == 0 {
			c.db.delete(key)
		}
		if deleted > 0 {
			go hashTypeBgResize(c.db, key)
		}
	default:
		panic("Unknown sorted set encoding")
	}

	// Step 4: Notifications and reply.
	if deleted > 0 {
		if keyremoved {

		}
	}
	c.addReplyLongLong(int64(deleted))
}

func zrangeCommand(c *client) {
	zrangeGenericCommand(c, false)
}

func zrevrangeCommand(c *client) {
	zrangeGenericCommand(c, true)
}

func zrangeGenericCommand(c *client, reverse bool) {
	var (
		start, end     int64
		ok, withscores bool
	)
	if start, ok = c.getLongLongOrReply(c.argv[2], nil); !ok {
		return
	}
	if end, ok = c.getLongLongOrReply(c.argv[3], nil); !ok {
		return
	}

	if c.argc == 5 && strings.EqualFold(string(c.argv[4]), "WITHSCORES") {
		withscores = true
	} else if c.argc >= 5 {
		c.addReply(shared.syntaxerr)
		return
	}

	if !c.db.lockKeyRead(c.argv[1]) { // expired
		c.addReply(shared.emptymultibulk)
		return
	}
	zobj, ok := c.getZsetOrReply(c.db.lookupKeyRead(c.argv[1]), shared.emptymultibulk)
	if !ok || zobj == nil {
		return
	}

	// Sanitize indexes.
	llen := int64(zsetLength(zobj))
	if start < 0 {
		start = llen + start
	}
	if end < 0 {
		end = llen + end
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
	rangelen := end - start + 1
	replylen := int(rangelen)
	if withscores {
		replylen *= 2
	}
	c.addReplyMultiBulkLen(replylen)

	switch z := zobj.(type) {
	case *zset:
		var ln *zskiplistNode
		// Check if starting point is trivial, before doing log(N) lookup.
		if reverse {
			ln = z.zsl.tail
			if start > 0 {
				ln = z.zsl.getElementByRank(uint(llen - start))
			}
		} else {
			ln = z.zsl.header.level[0].forward
			if start > 0 {
				ln = z.zsl.getElementByRank(uint(start + 1))
			}
		}

		for rangelen > 0 {
			rangelen--
			c.addReplyBulk(ln.ele)
			if withscores {
				c.addReplyDouble(ln.score)
			}
			if reverse {
				ln = ln.backward
			} else {
				ln = ln.level[0].forward
			}
		}
	default:
		panic("Unknown sorted set encoding")
	}
}

func zrangebyscoreCommand(c *client) {
	genericZrangebyscoreCommand(c, false)
}

func zrevrangebyscoreCommand(c *client) {
	genericZrangebyscoreCommand(c, true)
}

func genericZrangebyscoreCommand(c *client, reverse bool) {
	var minidx, maxidx int
	if reverse {
		maxidx = 2
		minidx = 3
	} else {
		minidx = 2
		maxidx = 3
	}

	zrange, ok := zslParseRange(c.argv[minidx], c.argv[maxidx])
	if !ok {
		c.addReplyError([]byte("min or max is not a float"))
		return
	}

	// Parse optional extra arguments. Note that ZCOUNT will exactly have
	// 4 arguments, so we'll never enter the following code path.
	var withscores bool
	var limit int64 = -1
	var offset int64 = 0
	if c.argc > 4 {
		remaining := c.argc - 4
		pos := 4

		for remaining > 0 {
			if remaining >= 1 && strings.EqualFold(string(c.argv[pos]), "WITHSCORES") {
				pos++
				remaining--
				withscores = true
			} else if remaining >= 3 && strings.EqualFold(string(c.argv[pos]), "LIMIT") {
				var ok bool
				if offset, ok = c.getLongLongOrReply(c.argv[pos+1], nil); !ok {
					return
				}
				if limit, ok = c.getLongLongOrReply(c.argv[pos+2], nil); !ok {
					return
				}
				pos += 3
				remaining -= 3
			} else {
				c.addReply(shared.syntaxerr)
				return
			}
		}
	}

	// Ok, lookup the key and get the range
	if !c.db.lockKeyRead(c.argv[1]) { // expired
		c.addReply(shared.emptymultibulk)
		return
	}

	zobj, ok := c.getZsetOrReply(c.db.lookupKeyRead(c.argv[1]), shared.emptymultibulk)
	if !ok || zobj == nil {
		return
	}

	rangelen := 0
	switch z := zobj.(type) {
	case *zset:
		zsl := z.zsl
		var ln *zskiplistNode
		if reverse {
			ln = zsl.lastInRange(&zrange)
		} else {
			ln = zsl.firstInRange(&zrange)
		}

		if ln == nil {
			c.addReply(shared.emptymultibulk)
			return
		}

		// We don't know in advance how many matching elements there are in the
		// list, so we push this object that will represent the multi-bulk
		// length in the output buffer, and will "fix" it later
		c.addDeferredMultiBulkLength()

		// If there is an offset, just traverse the number of elements without
		// checking the score because that is done in the next loop.
		for ln != nil && offset != 0 {
			if reverse {
				ln = ln.backward
			} else {
				ln = ln.level[0].forward
			}
			offset--
		}

		for ln != nil && limit != 0 {
			// Abort when the node is no longer in range.
			if reverse {
				if !zslValueGteMin(ln.score, &zrange) {
					break
				}
			} else {
				if !zslValueLteMax(ln.score, &zrange) {
					break
				}
			}
			rangelen++
			c.addReplyBulk(ln.ele)

			if withscores {
				c.addReplyDouble(ln.score)
			}

			// Move to next node
			if reverse {
				ln = ln.backward
			} else {
				ln = ln.level[0].forward
			}
			limit--
		}
	default:
		panic("Unknown sorted set encoding")
	}

	if withscores {
		rangelen *= 2
	}
	c.setDeferredMultiBulkLength(rangelen)
}

func zcountCommand(c *client) {
	key := c.argv[1]

	// Parse the range arguments
	zrange, ok := zslParseRange(c.argv[2], c.argv[3])
	if !ok {
		c.addReplyError([]byte("min or max is not a float"))
		return
	}

	// Lookup the sorted set
	if !c.db.lockKeyRead(key) { // expired
		c.addReply(shared.czero)
		return
	}
	var count uint
	if zobj, ok := c.getZsetOrReply(c.db.lookupKeyRead(key), shared.czero); ok && zobj != nil {
		switch zs := zobj.(type) {
		case *zset:
			// Find first element in range
			zn := zs.zsl.firstInRange(&zrange)
			// Use rank of first element, if any, to determine preliminary count
			if zn != nil {
				rank, _ := zs.zsl.getRank(zn.score, zn.ele)
				count = zs.zsl.length - (rank - 1)
				// Find last element in range
				zn = zs.zsl.lastInRange(&zrange)
				// Use rank of last element, if any, to determine the actual count
				if zn != nil {
					rank, _ := zs.zsl.getRank(zn.score, zn.ele)
					count -= (zs.zsl.length - rank)
				}
			}
		default:
			panic("Unknown sorted set encoding")
		}
	}
	c.addReplyLongLong(int64(count))
}

func zlexcountCommand(c *client) {
	key := c.argv[1]

	// Parse the range arguments
	zlexrange, ok := zslParseLexRange(c.argv[2], c.argv[3])
	if !ok {
		c.addReplyError([]byte("min or max not valid string range item"))
		return
	}

	// Lookup the sorted set
	if !c.db.lockKeyRead(key) { // expired
		c.addReply(shared.czero)
		return
	}
	var count uint
	if zobj, ok := c.getZsetOrReply(c.db.lookupKeyRead(key), shared.czero); ok && zobj != nil {
		switch zs := zobj.(type) {
		case *zset:
			// Find first element in range
			zn := zs.zsl.firstInLexRange(&zlexrange)
			// Use rank of first element, if any, to determine preliminary count
			if zn != nil {
				rank, _ := zs.zsl.getRank(zn.score, zn.ele)
				count = (zs.zsl.length - (rank - 1))

				// Find last element in range
				zn = zs.zsl.lastInLexRange(&zlexrange)

				// Use rank of last element, if any, to determine the actual count
				if zn != nil {
					rank, _ = zs.zsl.getRank(zn.score, zn.ele)
					count -= (zs.zsl.length - rank)
				}
			}
		default:
			panic("Unknown sorted set encoding")
		}
	}
	c.addReplyLongLong(int64(count))
}

func zrangebylexCommand(c *client) {
	genericZrangebylexCommand(c, false)
}

func zrevrangebylexCommand(c *client) {
	genericZrangebylexCommand(c, true)
}

func genericZrangebylexCommand(c *client, reverse bool) {
	// Parse the range arguments.
	var minidx, maxidx int
	if reverse {
		maxidx = 2
		minidx = 3
	} else {
		minidx = 2
		maxidx = 3
	}
	zlexrange, ok := zslParseLexRange(c.argv[minidx], c.argv[maxidx])
	if !ok {
		c.addReplyError([]byte("min or max not valid string range item"))
		return
	}

	// Parse optional extra arguments. Note that ZCOUNT will exactly have
	// 4 arguments, so we'll never enter the following code path.
	var limit int64 = -1
	var offset int64 = 0
	if c.argc > 4 {
		remaining := c.argc - 4
		pos := 4

		for remaining > 0 {
			if remaining >= 3 && strings.EqualFold(string(c.argv[pos]), "LIMIT") {
				var ok bool
				if offset, ok = c.getLongLongOrReply(c.argv[pos+1], nil); !ok {
					return
				}
				if limit, ok = c.getLongLongOrReply(c.argv[pos+2], nil); !ok {
					return
				}
				pos += 3
				remaining -= 3
			} else {
				c.addReply(shared.syntaxerr)
				return
			}
		}
	}

	// Ok, lookup the key and get the range
	if !c.db.lockKeyRead(c.argv[1]) { // expired
		c.addReply(shared.emptymultibulk)
		return
	}

	zobj, ok := c.getZsetOrReply(c.db.lookupKeyRead(c.argv[1]), shared.emptymultibulk)
	if !ok || zobj == nil {
		return
	}

	rangelen := 0
	switch z := zobj.(type) {
	case *zset:
		zsl := z.zsl
		var ln *zskiplistNode
		if reverse {
			ln = zsl.lastInLexRange(&zlexrange)
		} else {
			ln = zsl.firstInLexRange(&zlexrange)
		}

		if ln == nil {
			c.addReply(shared.emptymultibulk)
			return
		}

		// We don't know in advance how many matching elements there are in the
		// list, so we push this object that will represent the multi-bulk
		// length in the output buffer, and will "fix" it later
		c.addDeferredMultiBulkLength()

		// If there is an offset, just traverse the number of elements without
		// checking the score because that is done in the next loop.
		for ln != nil && offset != 0 {
			if reverse {
				ln = ln.backward
			} else {
				ln = ln.level[0].forward
			}
			offset--
		}

		for ln != nil && limit != 0 {
			// Abort when the node is no longer in range.
			if reverse {
				if !zslLexValueGteMin(ln.ele, &zlexrange) {
					break
				}
			} else {
				if !zslLexValueLteMax(ln.ele, &zlexrange) {
					break
				}
			}
			rangelen++
			c.addReplyBulk(ln.ele)

			// Move to next node
			if reverse {
				ln = ln.backward
			} else {
				ln = ln.level[0].forward
			}
			limit--
		}
	default:
		panic("Unknown sorted set encoding")
	}
	c.setDeferredMultiBulkLength(rangelen)
}

func zcardCommand(c *client) {
	if !c.db.lockKeyRead(c.argv[1]) { // expired
		c.addReply(shared.czero)
		return
	}

	if zobj, ok := c.getZsetOrReply(c.db.lookupKeyRead(c.argv[1]), shared.czero); ok && zobj != nil {
		c.addReplyLongLong(int64(zsetLength(zobj)))
	}
}

func zscoreCommand(c *client) {
	if !c.db.lockKeyRead(c.argv[1]) { // expired
		c.addReply(shared.nullbulk)
		return
	}

	if zobj, ok := c.getZsetOrReply(c.db.lookupKeyRead(c.argv[1]), shared.nullbulk); ok && zobj != nil {
		if score, ok := zsetScore(zobj, c.argv[2]); ok {
			c.addReplyDouble(score)
		} else {
			c.addReply(shared.nullbulk)
		}
	}
}

func zrankCommand(c *client) {
	zrankGenericCommand(c, false)
}

func zrevrankCommand(c *client) {
	zrankGenericCommand(c, true)
}

func zrankGenericCommand(c *client, reverse bool) {
	if !c.db.lockKeyRead(c.argv[1]) { // expired
		c.addReply(shared.nullbulk)
		return
	}

	if zobj, ok := c.getZsetOrReply(c.db.lookupKeyRead(c.argv[1]), shared.nullbulk); ok && zobj != nil {
		if rank, ok := zsetRank(zobj, c.argv[2], reverse); ok {
			c.addReplyLongLong(int64(rank))
		} else {
			c.addReply(shared.nullbulk)
		}
	}
}

func zunionstoreCommand(c *client) {
	zunionInterGenericCommand(c, c.argv[1], SET_OP_UNION)
}

func zinterstoreCommand(c *client) {
	zunionInterGenericCommand(c, c.argv[1], SET_OP_INTER)
}

func zunionInterGenericCommand(c *client, dstkey []byte, op int) {
	// expect setnum input keys to be given
	setnum := 0
	if num, ok := c.getLongLongOrReply(c.argv[2], nil); !ok {
		return
	} else {
		setnum = int(num)
	}

	if setnum < 1 {
		c.addReplyError([]byte("at least 1 input key is needed for ZUNIONSTORE/ZINTERSTORE"))
		return
	}

	// test if the expected number of keys would overflow
	if setnum > c.argc-3 {
		c.addReply(shared.syntaxerr)
		return
	}
	// Tricky here: lock dstkey and input keys in one lockKeysWrite call to
	// avoid deadlock.
	// TODO: since this is write command, key locks aquired will automatically
	// become wlock, which is actually not necessary here.
	keys := make([][]byte, setnum+1)
	keys[0] = dstkey
	copy(keys[1:], c.argv[3:])
	c.db.lockKeysWrite(keys, 1)

	src := make([]zsetopsrc, setnum)
	for i := 0; i < setnum; i++ {
		if zobj, ok := c.getSetZsetOrReply(c.db.lookupKeyWrite(c.argv[i+3]), nil); !ok {
			return
		} else {
			src[i].subject = zobj
			// Default all weights to 1.
			src[i].weight = 1.0
		}
	}
	// parse optional extra arguments
	j := setnum + 3
	remaining := c.argc - j
	aggregate := REDIS_AGGR_SUM
	for remaining > 0 {
		if remaining > setnum && strings.EqualFold(string(c.argv[j]), "weights") {
			j++
			remaining--
			for i := 0; i < setnum; i, j, remaining = i+1, j+1, remaining-1 {
				var ok bool
				if src[i].weight, ok = c.getLongDoubleOrReply(c.argv[j],
					[]byte("-ERR weight value is not a float\r\n")); !ok {
					return
				}
			}
		} else if remaining >= 2 && strings.EqualFold(string(c.argv[j]), "aggregate") {
			j++
			remaining--
			if strings.EqualFold(string(c.argv[j]), "sum") {
				aggregate = REDIS_AGGR_SUM
			} else if strings.EqualFold(string(c.argv[j]), "min") {
				aggregate = REDIS_AGGR_MIN
			} else if strings.EqualFold(string(c.argv[j]), "max") {
				aggregate = REDIS_AGGR_MAX
			} else {
				c.addReply(shared.syntaxerr)
				return
			}
			j++
			remaining--
		} else {
			c.addReply(shared.syntaxerr)
			return
		}
	}

	// sort sets from the smallest to largest, this will improve our
	// algorithm's performance
	sort.Sort(zsetops(src))
	dstzset := zsetCreate()
	zval := new(zsetopval)
	if op == SET_OP_INTER {
		// Skip everything if the smallest input is empty.
		if src[0].zuiLength() > 0 {
			// Precondition: as src[0] is non-empty and the inputs are ordered
			// by size, all src[i > 0] are non-empty too.
			src[0].zuiInitIterator()
			for src[0].zuiNext(zval) {
				score := src[0].weight * zval.score
				if math.IsNaN(score) {
					score = 0
				}

				j := 0
				for j = 1; j < setnum; j++ {
					// It is not safe to access the zset we are
					// iterating, so explicitly check for equal object.
					if src[j].subject == src[0].subject {
						value := zval.score * src[j].weight
						score = zuinionInterAggregate(score, value, aggregate)
					} else if value, ok := src[j].zuiFind(zval); ok {
						value *= src[j].weight
						score = zuinionInterAggregate(score, value, aggregate)
					} else {
						break
					}
				}

				// Only continue when present in every input.
				if j == setnum {
					tmp := zval.zuiNewSdsFromValue()
					dstzset.zsl.insert(score, tmp)
					dstzset.dict.set(tmp, score)
				}
			}
		}
	} else if op == SET_OP_UNION {
		accumulator := NewDict(int(src[setnum-1].zuiLength()), 1024)
		// Step 1: Create a dictionary of elements -> aggregated-scores
		// by iterating one sorted set after the other.
		for i := 0; i < setnum; i++ {
			if src[i].zuiLength() == 0 {
				continue
			}
			src[i].zuiInitIterator()
			for src[i].zuiNext(zval) {
				// Initialize value
				score := src[i].weight * zval.score
				if math.IsNaN(score) {
					score = 0
				}
				// Search for this element in the accumulating dictionary.
				_, _, _, de := accumulator.find(zval.ele)
				if de == nil {
					tmp := zval.zuiNewSdsFromValue()
					accumulator.set(tmp, score)
				} else {
					txn("undo") {
					de.value = zuinionInterAggregate(de.value.(float64), score, aggregate)
					}
				}
			}
		}
		// Step 2: convert the dictionary into the final sorted set. Since we
		// directly store score in zset dict, we can directly use it.
		di := accumulator.getIterator()
		for de := di.next(); de != nil; de = di.next() {
			dstzset.zsl.insert(de.value.(float64), de.key)
		}
		txn("undo") {
		dstzset.dict = accumulator
		}
	} else {
		panic("Unknown operator")
	}
	c.db.delete(dstkey)
	if dstzset.zsl.length > 0 {
		c.db.setKey(shadowCopyToPmem(dstkey), dstzset)
		c.addReplyLongLong(int64(dstzset.zsl.length))
	} else {
		c.addReply(shared.czero)
	}
}

// ============== common zset api ====================
func zsetCreate() *zset {
	zs := pnew(zset)
	txn("undo") {
	zs.dict = NewDict(4, 4)
	zs.zsl = zslCreate()
	}
	return zs
}

func zsetAdd(c *client, zobj interface{}, score float64, ele []byte, flags int) (int, float64, int) {
	incr := (flags & ZADD_INCR) != 0
	nx := (flags & ZADD_NX) != 0
	xx := (flags & ZADD_XX) != 0
	flags = 0

	// NaN as input is an error regardless of all the other parameters.
	if math.IsNaN(score) {
		return 0, 0, flags
	}

	switch zs := zobj.(type) {
	case *zset:
		_, _, _, de := zs.dict.find(ele)
		if de != nil {
			// NX? Return, same element already exists.
			if nx {
				return 1, 0, flags | ZADD_NOP
			}
			curscore := de.value.(float64)

			// NX? Return, same element already exists.
			if incr {
				score += curscore
				if math.IsNaN(score) {
					return 0, 0, flags
				}
			}

			// Remove and re-insert when score changes.
			if score != curscore {
				node := zs.zsl.delete(curscore, ele)
				zs.zsl.insert(score, node.ele)
				txn("undo") {
				// Change dictionary value to directly store score value instead
				// of pointer to score in zsl.
				de.value = score
				}
				flags |= ZADD_UPDATED
			}
			return 1, score, flags
		} else if !xx {
			ele = shadowCopyToPmem(ele)
			zs.zsl.insert(score, ele)
			zs.dict.set(ele, score)
			go hashTypeBgResize(c.db, c.argv[1])
			return 1, score, flags | ZADD_ADDED
		} else {
			return 1, 0, flags | ZADD_NOP
		}
	default:
		panic("Unknown sorted set encoding")
	}

}

func zsetDel(c *client, zobj interface{}, ele []byte) bool {
	switch zs := zobj.(type) {
	case *zset:
		de := zs.dict.delete(ele)
		if de != nil {
			score := de.value.(float64)
			retvalue := zs.zsl.delete(score, ele)
			if retvalue == nil {
				panic("Zset dict and skiplist does not match!")
			}
			// TODO: resize
			go hashTypeBgResize(c.db, c.argv[1])
			return true
		}
	default:
		panic("Unknown sorted set encoding")
	}
	return false
}

func zsetLength(zobj interface{}) uint {
	switch zs := zobj.(type) {
	case *zset:
		return zs.zsl.length
	default:
		panic("Unknown sorted set encoding")
	}
}

func zsetScore(zobj interface{}, ele []byte) (float64, bool) {
	if zobj == nil || ele == nil {
		return 0, false
	}
	switch zs := zobj.(type) {
	case *zset:
		_, _, _, de := zs.dict.find(ele)
		if de == nil {
			return 0, false
		} else {
			return de.value.(float64), true
		}
	default:
		panic("Unknown sorted set encoding")
	}
}

func zsetRank(zobj interface{}, ele []byte, reverse bool) (uint, bool) {
	switch zs := zobj.(type) {
	case *zset:
		_, _, _, de := zs.dict.find(ele)
		if de == nil {
			return 0, false
		} else {
			score := de.value.(float64)
			rank, _ := zs.zsl.getRank(score, ele)
			if reverse {
				return zs.zsl.length - rank, true
			} else {
				return rank - 1, true
			}
		}
	default:
		panic("Unknown sorted set encoding")
	}
}

// ============== common skiplist api ====================

func zslCreate() *zskiplist {
	zsl := pnew(zskiplist)
	txn("undo") {
	zsl.level = 1
	zsl.length = 0
	zsl.header = zslCreateNode(ZSKIPLIST_MAXLEVEL, 0, nil)
	}
	return zsl
}

func zslCreateNode(level int, score float64, ele []byte) *zskiplistNode {
	zn := pnew(zskiplistNode)
	txn("undo") {
	zn.score = score
	zn.ele = ele
	zn.level = pmake([]zskiplistLevel, level)
	}
	return zn
}

func zslRandomLevel() int {
	level := 1
	t := ZSKIPLIST_P * 0xFFFF
	ti := uint32(t)
	for rand.Uint32()&0xFFFF < ti {
		level++
	}
	if level < ZSKIPLIST_MAXLEVEL {
		return level
	} else {
		return ZSKIPLIST_MAXLEVEL
	}
}

func zslParseRange(min, max []byte) (spec zrangespec, ok bool) {
	if min[0] == '(' {
		if spec.min, ok = getLongDouble(min[1:]); !ok {
			return
		} else {
			spec.minex = true
		}
	} else {
		if spec.min, ok = getLongDouble(min); !ok {
			return
		}
	}

	if max[0] == '(' {
		if spec.max, ok = getLongDouble(max[1:]); !ok {
			return
		} else {
			spec.maxex = true
		}
	} else {
		if spec.max, ok = getLongDouble(max); !ok {
			return
		}
	}
	return
}

func zslValueGteMin(value float64, spec *zrangespec) bool {
	if spec.minex {
		return value > spec.min
	} else {
		return value >= spec.min
	}
}

func zslValueLteMax(value float64, spec *zrangespec) bool {
	if spec.maxex {
		return value < spec.max
	} else {
		return value <= spec.max
	}
}

func zslDeleteRangeByRank(zsl *zskiplist, start, end uint, dict *dict) uint {
	update := make([]*zskiplistNode, ZSKIPLIST_MAXLEVEL)
	x := zsl.header
	var traversed, removed uint
	for i := zsl.level - 1; i >= 0; i-- {
		for x.level[i].forward != nil && (traversed+x.level[i].span) < start {
			traversed += x.level[i].span
			x = x.level[i].forward
		}
		update[i] = x
	}
	traversed++
	x = x.level[0].forward
	for x != nil && traversed <= end {
		zsl.deleteNode(x, update)
		dict.delete(x.ele)
		removed++
		traversed++
		x = x.level[0].forward
	}
	return removed
}

func zslDeleteRangeByScore(zsl *zskiplist, zrange *zrangespec, dict *dict) uint {
	update := make([]*zskiplistNode, ZSKIPLIST_MAXLEVEL)
	x := zsl.header
	var removed uint
	for i := zsl.level - 1; i >= 0; i-- {
		for x.level[i].forward != nil {
			if (zrange.minex && x.level[i].forward.score <= zrange.min) ||
				(!zrange.minex && x.level[i].forward.score < zrange.min) {
				x = x.level[i].forward
			} else {
				break
			}
		}
		update[i] = x
	}
	x = x.level[0].forward
	for x != nil {
		if (zrange.maxex && x.score < zrange.max) ||
			(!zrange.maxex && x.score <= zrange.max) {
			zsl.deleteNode(x, update)
			dict.delete(x.ele)
			removed++
			x = x.level[0].forward
		} else {
			break
		}
	}
	return removed
}

func zslDeleteRangeByLex(zsl *zskiplist, lexrange *zlexrangespec, dict *dict) uint {
	update := make([]*zskiplistNode, ZSKIPLIST_MAXLEVEL)
	x := zsl.header
	var removed uint
	for i := zsl.level - 1; i >= 0; i-- {
		for x.level[i].forward != nil &&
			!zslLexValueGteMin(x.level[i].forward.ele, lexrange) {
			x = x.level[i].forward
		}
		update[i] = x
	}
	x = x.level[0].forward
	for x != nil && zslLexValueLteMax(x.ele, lexrange) {
		zsl.deleteNode(x, update)
		dict.delete(x.ele)
		removed++
		x = x.level[0].forward
	}
	return removed
}

func (zsl *zskiplist) insert(score float64, ele []byte) *zskiplistNode {
	update := make([]*zskiplistNode, ZSKIPLIST_MAXLEVEL)
	rank := make([]uint, ZSKIPLIST_MAXLEVEL)
	x := zsl.header
	for i := zsl.level - 1; i >= 0; i-- {
		// store rank that is crossed to reach the insert position
		if i == zsl.level-1 {
			rank[i] = 0
		} else {
			rank[i] = rank[i+1]
		}
		for x.level[i].forward != nil &&
			(x.level[i].forward.score < score ||
				(x.level[i].forward.score == score &&
					bytes.Compare(x.level[i].forward.ele, ele) < 0)) {
			rank[i] += x.level[i].span
			x = x.level[i].forward
		}
		update[i] = x
	}
	// we assume the element is not already inside, since we allow duplicated
	// scores, reinserting the same element should never happen since the
	// caller of zslInsert() should test in the hash table if the element is
	// already inside or not.
	txn("undo") {
	level := zslRandomLevel()
	if level > zsl.level {
		for i := zsl.level; i < level; i++ {
			rank[i] = 0
			update[i] = zsl.header
			update[i].level[i].span = zsl.length
		}
		zsl.level = level
	}
	x = zslCreateNode(level, score, ele)
	for i := 0; i < level; i++ {
		x.level[i].forward = update[i].level[i].forward
		update[i].level[i].forward = x

		// update span covered by update[i] as x is inserted here
		x.level[i].span = update[i].level[i].span - (rank[0] - rank[i])
		update[i].level[i].span = (rank[0] - rank[i]) + 1
	}

	// increment span for untouched levels
	for i := level; i < zsl.level; i++ {
		update[i].level[i].span++
	}

	if update[0] == zsl.header {
		x.backward = nil
	} else {
		x.backward = update[0]
	}
	if x.level[0].forward != nil {
		x.level[0].forward.backward = x
	} else {
		zsl.tail = x
	}
	zsl.length++
	}
	return x
}

func (zsl *zskiplist) delete(score float64, ele []byte) *zskiplistNode {
	update := make([]*zskiplistNode, ZSKIPLIST_MAXLEVEL)
	x := zsl.header

	for i := zsl.level - 1; i >= 0; i-- {
		for x.level[i].forward != nil &&
			(x.level[i].forward.score < score ||
				(x.level[i].forward.score == score &&
					bytes.Compare(x.level[i].forward.ele, ele) < 0)) {
			x = x.level[i].forward
		}
		update[i] = x
	}

	x = x.level[0].forward
	if x != nil && score == x.score && bytes.Compare(x.ele, ele) == 0 {
		zsl.deleteNode(x, update)
		return x
	}
	return nil
}

func (zsl *zskiplist) deleteNode(x *zskiplistNode, update []*zskiplistNode) {
	txn("undo") {
	for i := 0; i < zsl.level; i++ {
		if update[i].level[i].forward == x {
			update[i].level[i].span += x.level[i].span - 1
			update[i].level[i].forward = x.level[i].forward
		} else {
			update[i].level[i].span -= 1
		}
	}
	
	if x.level[0].forward != nil {
		x.level[0].forward.backward = x.backward
	} else {
		zsl.tail = x.backward
	}

	for zsl.level > 1 && zsl.header.level[zsl.level-1].forward == nil {
		zsl.level--
	}
	zsl.length--
	}
}

func (zsl *zskiplist) getElementByRank(rank uint) *zskiplistNode {
	x := zsl.header
	var traversed uint
	for i := zsl.level - 1; i >= 0; i-- {
		for x.level[i].forward != nil && (traversed+x.level[i].span) <= rank {
			traversed += x.level[i].span
			x = x.level[i].forward
		}
		if traversed == rank {
			return x
		}
	}
	return nil
}

func (zsl *zskiplist) firstInRange(zrange *zrangespec) *zskiplistNode {
	if !zsl.isInRange(zrange) {
		return nil
	}

	x := zsl.header
	for i := zsl.level - 1; i >= 0; i-- {
		for x.level[i].forward != nil &&
			!zslValueGteMin(x.level[i].forward.score, zrange) {
			x = x.level[i].forward
		}
	}

	x = x.level[0].forward
	if !zslValueLteMax(x.score, zrange) {
		return nil
	}
	return x
}

func (zsl *zskiplist) lastInRange(zrange *zrangespec) *zskiplistNode {
	if !zsl.isInRange(zrange) {
		return nil
	}

	x := zsl.header
	for i := zsl.level - 1; i >= 0; i-- {
		for x.level[i].forward != nil &&
			zslValueLteMax(x.level[i].forward.score, zrange) {
			x = x.level[i].forward
		}
	}

	if !zslValueGteMin(x.score, zrange) {
		return nil
	}
	return x
}

func (zsl *zskiplist) isInRange(zrange *zrangespec) bool {
	if zrange.min > zrange.max || (zrange.min == zrange.max && (zrange.minex || zrange.maxex)) {
		return false
	}
	x := zsl.tail
	if x == nil || !zslValueGteMin(x.score, zrange) {
		return false
	}
	x = zsl.header.level[0].forward
	if x == nil || !zslValueLteMax(x.score, zrange) {
		return false
	}
	return true
}

func (zsl *zskiplist) getRank(score float64, ele []byte) (uint, bool) {
	x := zsl.header
	var rank uint
	for i := zsl.level - 1; i >= 0; i-- {
		for x.level[i].forward != nil &&
			(x.level[i].forward.score < score ||
				(x.level[i].forward.score == score &&
					bytes.Compare(x.level[i].forward.ele, ele) <= 0)) {
			rank += x.level[i].span
			x = x.level[i].forward
		}

		if x.ele != nil && bytes.Compare(x.ele, ele) == 0 {
			return rank, true
		}
	}
	return 0, false
}

// ============== Lexicographic ranges ====================

func zslParseLexRange(min, max []byte) (spec zlexrangespec, ok bool) {
	if spec.min, spec.minex, ok = zslParseLexRangeItem(min); !ok {
		return
	}
	spec.max, spec.maxex, ok = zslParseLexRangeItem(max)
	return
}

func zslParseLexRangeItem(item []byte) ([]byte, bool, bool) {
	switch item[0] {
	case '+':
		if len(item) > 1 {
			return nil, false, false
		}
		return shared.maxstring, false, true
	case '-':
		if len(item) > 1 {
			return nil, false, false
		}
		return shared.minstring, false, true
	case '(':
		return item[1:], true, true
	case '[':
		return item[1:], false, true
	default:
		return nil, false, false
	}
}

func zslLexValueLteMax(value []byte, spec *zlexrangespec) bool {
	if spec.maxex {
		return cmplex(value, spec.max) < 0
	} else {
		return cmplex(value, spec.max) <= 0
	}
}

func zslLexValueGteMin(value []byte, spec *zlexrangespec) bool {
	if spec.minex {
		return cmplex(value, spec.min) > 0
	} else {
		return cmplex(value, spec.min) >= 0
	}
}

func cmplex(a, b []byte) int {
	// trick to compare underlying slice value
	if (len(a) > 0 && &a[0] == &shared.minstring[0]) || (len(b) > 0 && &b[0] == &shared.maxstring[0]) {
		return -1
	}
	if (len(a) > 0 && &a[0] == &shared.maxstring[0]) || (len(b) > 0 && &b[0] == &shared.minstring[0]) {
		return 1
	}
	return bytes.Compare(a, b)
}

func (zsl *zskiplist) firstInLexRange(zrange *zlexrangespec) *zskiplistNode {
	if !zsl.isInLexRange(zrange) {
		return nil
	}
	x := zsl.header
	for i := zsl.level - 1; i >= 0; i-- {
		for x.level[i].forward != nil &&
			!zslLexValueGteMin(x.level[i].forward.ele, zrange) {
			x = x.level[i].forward
		}
	}

	x = x.level[0].forward
	if !zslLexValueLteMax(x.ele, zrange) {
		return nil
	}
	return x
}

func (zsl *zskiplist) lastInLexRange(zrange *zlexrangespec) *zskiplistNode {
	if !zsl.isInLexRange(zrange) {
		return nil
	}
	x := zsl.header
	for i := zsl.level - 1; i >= 0; i-- {
		for x.level[i].forward != nil &&
			zslLexValueLteMax(x.level[i].forward.ele, zrange) {
			x = x.level[i].forward
		}
	}

	if !zslLexValueGteMin(x.ele, zrange) {
		return nil
	}
	return x
}

func (zsl *zskiplist) isInLexRange(zrange *zlexrangespec) bool {
	// Test for ranges that will always be empty.
	if cmplex(zrange.min, zrange.max) > 1 ||
		(bytes.Compare(zrange.min, zrange.max) == 0 && (zrange.minex || zrange.maxex)) {
		return false
	}
	x := zsl.tail
	if x == nil || !zslLexValueGteMin(x.ele, zrange) {
		return false
	}
	x = zsl.header.level[0].forward
	if x == nil || !zslLexValueLteMax(x.ele, zrange) {
		return false
	}
	return true
}

// ============== union inter helper functions ====================
func (ops zsetops) Len() int {
	return len(ops)
}

func (ops zsetops) Swap(i, j int) {
	ops[i], ops[j] = ops[j], ops[i]
}

func (ops zsetops) Less(i, j int) bool {
	return ops[i].zuiLength() < ops[j].zuiLength()
}

func (op *zsetopsrc) zuiLength() uint {
	switch s := op.subject.(type) {
	case *zset:
		return s.zsl.length
	case *dict:
		return uint(s.size())
	case nil:
		return 0
	default:
		panic("Unsupported type")
	}
}

func (op *zsetopsrc) zuiInitIterator() {
	switch s := op.subject.(type) {
	case *zset:
		op.iter = &zsetIter{zs: s,
			node: s.zsl.header.level[0].forward}
	case *dict:
		op.iter = s.getIterator()
	case nil:
		op.iter = nil
	default:
		panic("Unsupported type")
	}
}

func (op *zsetopsrc) zuiNext(target *zsetopval) bool {
	switch it := op.iter.(type) {
	case *zsetIter:
		if it.node == nil {
			return false
		}
		target.ele = it.node.ele
		target.score = it.node.score
		it.node = it.node.level[0].forward
		return true
	case *dictIterator:
		de := it.next()
		if de == nil {
			return false
		} else {
			target.ele = de.key
			target.score = 1
			return true
		}
	case nil:
		return false
	default:
		panic("Unsupported type")
	}
}

func zuinionInterAggregate(val1, val2 float64, aggregate int) float64 {
	if aggregate == REDIS_AGGR_SUM {
		target := val1 + val2
		if math.IsNaN(target) {
			return 0
		} else {
			return target
		}
	} else if aggregate == REDIS_AGGR_MIN {
		if val1 < val2 {
			return val1
		} else {
			return val2
		}
	} else if aggregate == REDIS_AGGR_MAX {
		if val1 < val2 {
			return val2
		} else {
			return val1
		}
	} else {
		panic("Unknown ZUNION/INTER aggregate type")
	}
	return 0
}

func (op *zsetopsrc) zuiFind(val *zsetopval) (float64, bool) {
	switch s := op.subject.(type) {
	case *zset:
		_, _, _, de := s.dict.find(val.ele)
		if de == nil {
			return 0, false
		} else {
			return de.value.(float64), true
		}
	case *dict:
		_, _, _, de := s.find(val.ele)
		if de == nil {
			return 0, false
		} else {
			return 1, true
		}
	case nil:
		return 0, false
	default:
		panic("Unsupported type")
	}
}

func (val *zsetopval) zuiNewSdsFromValue() []byte {
	return shadowCopyToPmem(val.ele)
}
