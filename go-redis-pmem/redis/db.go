///////////////////////////////////////////////////////////////////////
// Copyright 2018-2019 VMware, Inc.
// SPDX-License-Identifier: BSD-2-Clause
///////////////////////////////////////////////////////////////////////

package redis

import (
	"time"

	"github.com/vmware/go-pmem-transaction/transaction"
)

// General db locking order:
// expire -> dict
// rehash Lock -> table Lock -> bucket Lock
// table0 -> table1
// bucket id ascending

var expired chan []byte = make(chan []byte, 100)

func (db *redisDb) Cron() {
	go db.dict.Cron(100 * time.Millisecond)
	go db.expire.Cron(100 * time.Millisecond)
	go db.expireCron(10 * time.Millisecond)
}

// active expire (only check table 0 for simplicity)
func (db *redisDb) expireCron(sleep time.Duration) {
	i := 0
	ticker := time.NewTicker(sleep)
	for {
		select {
		case key := <-expired:
			txn("undo") {
			db.lockKeyWrite(key) // lockKeyWrite calls expireIfNeeded.
			}
		case <-ticker.C:
			txn("undo") {
			db.expire.lock.RLock()
			mask := db.expire.tab[0].mask
			i = i & mask
			s := db.expire.shard(i)
			db.expire.lockShard(0, s)
			e := db.expire.tab[0].bucket[i]
			for e != nil {
				when := e.value.(int64)
				now := time.Now().UnixNano()
				if when <= now {
					db.dict.lockKey(e.key)
					db.delete(e.key)
					e = e.next
					// only delete one expire key in each transaction to prevent
					// deadlock.
					break
				}
				e = e.next
			}
			if e == nil { // finished checking current bucket
				i++
			}
			}
		}
	}
}

func (db *redisDb) swizzle() {
	db.dict.swizzle()
	db.expire.swizzle()
}

func existsCommand(c *client) {
	var count int64

	alive := c.db.lockKeysRead(c.argv[1:], 1)

	for i, key := range c.argv[1:] {
		if alive[i] {
			if c.db.lookupKeyRead(key) != nil {
				count++
			}
		}
	}
	c.addReplyLongLong(count)
}

func delCommand(c *client) {
	var count int64

	c.db.lockKeysWrite(c.argv[1:], 1)

	for _, key := range c.argv[1:] {
		c.db.expireIfNeeded(key)
		if c.db.delete(key) {
			count++
		}
	}
	c.addReplyLongLong(count)
}

func dbsizeCommand(c *client) {
	c.db.dict.lockAllKeys()
	c.addReplyLongLong(int64(c.db.dict.size()))
}

func flushdbCommand(c *client) {
	c.db.lockTablesWrite()
	c.db.expire.empty()
	c.db.dict.empty()
	c.addReply(shared.ok)
}

func selectCommand(c *client) {
	// TODO: not implemented
	c.addReply(shared.ok)
}

func randomkeyCommand(c *client) {
	c.db.dict.lockAllKeys()
	if de := c.db.randomKey(); de != nil {
		c.addReplyBulk(de.key)
	} else {
		c.addReply(shared.nullbulk)
	}
}

func (db *redisDb) lockKeyWrite(key []byte) {
	db.expire.lockKey(key)
	db.dict.lockKey(key)
	db.expireIfNeeded(key)
}

func (db *redisDb) lockKeyRead(key []byte) bool {
	db.expire.lockKey(key)
	db.dict.lockKey(key)
	return db.checkLiveKey(key)
}

func (db *redisDb) lockKeysWrite(keys [][]byte, stride int) {
	db.expire.lockKeys(keys, stride)
	db.dict.lockKeys(keys, stride)
	for i := 0; i < len(keys)/stride; i++ {
		db.expireIfNeeded(keys[i*stride])
	}
}

func (db *redisDb) lockKeysRead(keys [][]byte, stride int) []bool {
	db.expire.lockKeys(keys, stride)
	db.dict.lockKeys(keys, stride)
	return db.checkLiveKeys(keys, stride)
}

func (db *redisDb) lockTablesWrite() {
	txn("undo") {
	db.expire.rehashLock.Lock()
	db.expire.lock.Lock()
	db.dict.rehashLock.Lock()
	db.dict.lock.Lock()
	}
}

func (db *redisDb) checkLiveKeys(keys [][]byte, stride int) []bool {
	alive := make([]bool, len(keys))
	for i := 0; i < len(keys)/stride; i++ {
		alive[i*stride] = db.checkLiveKey(keys[i*stride])
	}
	return alive
}

func (db *redisDb) checkLiveKey(key []byte) bool {
	when := db.getExpire(key)
	if when < 0 {
		return true
	}
	now := time.Now().UnixNano()
	if now < when {
		return true
	}
	expired <- key
	return false
}

func (db *redisDb) lookupKeyWrite(key []byte) interface{} {
	return db.lookupKey(key)
}

func (db *redisDb) lookupKeyRead(key []byte) interface{} {
	return db.lookupKey(key)
}

func (db *redisDb) lookupKey(key []byte) interface{} {
	_, _, _, e := db.dict.find(key)
	if e != nil {
		return e.value
	}
	return nil
}

func (db *redisDb) randomKey() *entry {
	for {
		entry := db.dict.randomKey()
		if entry == nil {
			return entry
		}
		if db.checkLiveKey(entry.key) {
			return entry
		}
	}
}

// key and value data should be in pmem
func (db *redisDb) setKey(key []byte, value interface{}) (insert bool) {
	db.removeExpire(key)
	return db.dict.set(key, value)
}

func (db *redisDb) delete(key []byte) bool {
	db.expire.delete(key)
	return (db.dict.delete(key) != nil)
}

func expireCommand(c *client) {
	expireGeneric(c, time.Now(), time.Second)
}

func expireatCommand(c *client) {
	expireGeneric(c, time.Unix(0, 0), time.Second)
}

func pexpireCommand(c *client) {
	expireGeneric(c, time.Now(), time.Millisecond)
}

func pexpireatCommand(c *client) {
	expireGeneric(c, time.Unix(0, 0), time.Millisecond)
}

func expireGeneric(c *client, base time.Time, d time.Duration) {
	when, ok := c.getLongLongOrReply(c.argv[2], nil)
	if !ok {
		return
	}

	expire := base.Add(time.Duration(when) * d)

	c.db.lockKeyWrite(c.argv[1])

	if c.db.lookupKeyWrite(c.argv[1]) == nil {
		c.addReply(shared.czero)
		return
	}

	// TODO: Expire with negative ttl or with timestamp into the past should
	// never executed as a DEL when load AOF or in the context of a slave.
	if expire.Before(time.Now()) {
		c.db.delete(c.argv[1])
		c.addReply(shared.cone)
		return
	} else {
		// int64 value should be inlined in interface, therefore should also be
		// persisted after set.
		c.db.setExpire(c.argv[1], expire.UnixNano())
		c.addReply(shared.cone)
		return
	}
}

func (db *redisDb) setExpire(key []byte, expire interface{}) {
	_, _, _, e := db.dict.find(key) // share the main dict key to save space
	if e == nil {
		panic("Trying set expire on non-existing key!")
	}
	db.expire.set(e.key, expire)
}

func (db *redisDb) removeExpire(key []byte) bool {
	return db.expire.delete(key) != nil
}

// tx must be writable
func (db *redisDb) expireIfNeeded(key []byte) {
	when := db.getExpire(key)
	if when < 0 {
		return
	}
	now := time.Now().UnixNano()
	if now < when {
		return
	}
	db.delete(key)
}

func (db *redisDb) getExpire(key []byte) int64 {
	_, _, _, e := db.expire.find(key)
	if e == nil {
		return -1
	}

	switch value := e.value.(type) {
	case int64:
		return value
	default:
		panic("Expire value type error!")
	}
}

func ttlCommand(c *client) {
	ttlGeneric(c, false)
}

func pttlCommand(c *client) {
	ttlGeneric(c, true)
}

func ttlGeneric(c *client, ms bool) {
	var ttl int64 = -1

	c.db.expire.lockKey(c.argv[1])
	c.db.dict.lockKey(c.argv[1])

	if c.db.lookupKeyRead(c.argv[1]) == nil {
		c.addReplyLongLong(-2)
		return
	}

	expire := c.db.getExpire(c.argv[1])
	if expire > 0 {
		ttl = expire - time.Now().UnixNano()
		if ttl < 0 {
			ttl = 0
		}
	}
	if ttl == -1 {
		c.addReplyLongLong(-1)
	} else {
		if ms {
			ttl = (ttl + 500000) / 1000000
		} else {
			ttl = (ttl + 500000000) / 1000000000
		}
		c.addReplyLongLong(ttl)
	}
}

func persistCommand(c *client) {
	c.db.lockKeyWrite(c.argv[1])
	_, _, _, e := c.db.dict.find(c.argv[1])
	if e == nil {
		c.addReply(shared.czero)
	} else {
		if c.db.removeExpire(c.argv[1]) {
			c.addReply(shared.cone)
		} else {
			c.addReply(shared.czero)
		}
	}
}
