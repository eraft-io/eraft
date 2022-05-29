///////////////////////////////////////////////////////////////////////
// Copyright 2018-2019 VMware, Inc.
// SPDX-License-Identifier: BSD-2-Clause
///////////////////////////////////////////////////////////////////////

package redis

import (
	"sort"
)

type (
	setslice    []interface{}
	revsetslice []interface{}

	setTypeIterator struct {
		subject interface{}
		di      *dictIterator
		ii      int // intset iterator
	}
)

const (
	SPOP_MOVE_STRATEGY_MUL       = 5
	SRANDMEMBER_SUB_STRATEGY_MUL = 3
)

// ============== set commands ====================
func saddCommand(c *client) {
	c.db.lockKeyWrite(c.argv[1])
	var added int64
	if set, ok := c.getSetOrReply(c.db.lookupKeyWrite(c.argv[1]), nil); !ok {
		return
	} else {
		if set == nil {
			set = setTypeCreate(c.argv[2])
			c.db.setKey(shadowCopyToPmem(c.argv[1]), set)
		}
		for j := 2; j < c.argc; j++ {
			if setTypeAdd(c, c.argv[1], set, c.argv[j]) {
				added++
			}
		}
	}
	c.addReplyLongLong(added)
}

func sremCommand(c *client) {
	c.db.lockKeyWrite(c.argv[1])
	var deleted int64
	set, ok := c.getSetOrReply(c.db.lookupKeyWrite(c.argv[1]), shared.czero)
	if !ok || set == nil {
		return
	} else {
		for j := 2; j < c.argc; j++ {
			if setTypeRemove(c, c.argv[1], set, c.argv[j]) {
				deleted++
				if setTypeSize(set) == 0 {
					c.db.delete(c.argv[1])
					break
				}
			}
		}
	}
	c.addReplyLongLong(deleted)
}

func smoveCommand(c *client) {
	c.db.lockKeysWrite(c.argv[1:3], 1)
	var srcset, dstset interface{}
	ele := c.argv[3]
	ok := false
	// If the source key does not exist return 0
	if srcset, ok = c.getSetOrReply(c.db.lookupKeyWrite(c.argv[1]), shared.czero); !ok || srcset == nil {
		return
	}
	// If the destination key is set and has the wrong type, return with an error.
	if dstset, ok = c.getSetOrReply(c.db.lookupKeyWrite(c.argv[2]), nil); !ok {
		return
	}

	// If srcset and dstset are equal, SMOVE is a no-op
	if dstset == srcset {
		if setTypeIsMember(srcset, ele) {
			c.addReply(shared.cone)
		} else {
			c.addReply(shared.czero)
		}
		return
	}

	// If the element cannot be removed from the src set, return 0.
	if !setTypeRemove(c, c.argv[1], srcset, ele) {
		c.addReply(shared.czero)
		return
	}

	// Remove the src set from the database when empty
	if setTypeSize(srcset) == 0 {
		c.db.delete(c.argv[1])
	}

	// Create the destination set when it doesn't exist
	if dstset == nil {
		dstset = setTypeCreate(ele)
		c.db.setKey(shadowCopyToPmem(c.argv[2]), dstset)
	}

	// An extra key has changed when ele was successfully added to dstset
	if setTypeAdd(c, c.argv[2], dstset, ele) {
		// notify key space event
	}
	c.addReply(shared.cone)
}

func spopCommand(c *client) {
	if c.argc == 3 {
		spopWithCountCommand(c)
		return
	} else if c.argc > 3 {
		c.addReply(shared.syntaxerr)
		return
	}

	// Make sure a key with the name inputted exists, and that it's type is
	// indeed a set
	c.db.lockKeyWrite(c.argv[1])
	set, ok := c.getSetOrReply(c.db.lookupKeyWrite(c.argv[1]), shared.nullbulk)
	if !ok || set == nil {
		return
	}

	// Get a random element from the set
	ele := setTypeRandomElement(set)

	// Remove the element from the set
	setTypeRemove(c, c.argv[1], set, ele)

	// Add the element to the reply
	reply, _ := getString(ele)
	c.addReplyBulk(reply)

	// Delete the set if it's empty
	if setTypeSize(set) == 0 {
		c.db.delete(c.argv[1])
	}
}

func spopWithCountCommand(c *client) {
	// Get the count argument
	ll, ok := c.getLongLongOrReply(c.argv[2], nil)
	if !ok {
		return
	} else if ll < 0 {
		c.addReply(shared.outofrangeerr)
		return
	}
	count := int(ll)

	// Make sure a key with the name inputted exists, and that it's type is
	// indeed a set. Otherwise, return nil
	c.db.lockKeyWrite(c.argv[1])
	set, ok2 := c.getSetOrReply(c.db.lookupKeyWrite(c.argv[1]), shared.nullbulk)
	if !ok2 || set == nil {
		return
	}

	// If count is zero, serve an empty multibulk ASAP to avoid special
	// cases later.
	if count == 0 {
		c.addReply(shared.emptymultibulk)
		return
	}

	size := setTypeSize(set)

	// CASE 1:
	// The number of requested elements is greater than or equal to
	// the number of elements inside the set: simply return the whole set.
	if count >= size {
		// We just return the entire set
		// do not lock key again
		sunionDiffGenericCommand(c, c.argv[1:2], nil, SET_OP_UNION_NOLOCK)

		// Delete the set as it is now empty
		c.db.delete(c.argv[1])
		return
	}

	// Case 2 and 3 require to replicate SPOP as a set of SREM commands.
	// Prepare our replication argument vector. Also send the array length
	// which is common to both the code paths.
	c.addReplyMultiBulkLen(count)

	// If we are here, the number of requested elements is less than the
	// number of elements inside the set. Also we are sure that count < size.
	// Use two different strategies.
	//
	// CASE 2: The number of elements to return is small compared to the
	// set size. We can just extract random elements and return them to
	// the set.
	remaining := size - count
	if remaining*SPOP_MOVE_STRATEGY_MUL > count {
		for ; count > 0; count-- {
			// Emit and remove.
			ele := setTypeRandomElement(set)
			setTypeRemove(c, c.argv[1], set, ele)
			reply, _ := getString(ele)
			c.addReplyBulk(reply)
		}
	} else {
		// CASE 3: The number of elements to return is very big, approaching
		// the size of the set itself. After some time extracting random elements
		// from such a set becomes computationally expensive, so we use
		// a different strategy, we extract random elements that we don't
		// want to return (the elements that will remain part of the set),
		// creating a new set as we do this (that will be stored as the original
		// set). Then we return the elements left in the original set and
		// release it.

		newset := setTypeCreate(nil)
		for ; remaining > 0; remaining-- {
			ele := setTypeRandomElement(set)
			setTypeAdd(c, nil, newset, ele)
			setTypeRemove(c, nil, set, ele)
		}

		// Assign the new set as the key value.
		// No need to copy argv[1] into pmem as we already know key exists in db
		c.db.setKey(c.argv[1], newset)

		// Tranfer the old set to the client.
		si := setTypeInitIterator(set)
		for ele := setTypeNext(si); ele != nil; ele = setTypeNext(si) {
			reply, _ := getString(ele)
			c.addReplyBulk(reply)
		}
	}
}

func srandmemberCommand(c *client) {
	if c.argc == 3 {
		srandmemberWithCountCommand(c)
		return
	} else if c.argc > 3 {
		c.addReply(shared.syntaxerr)
		return
	}

	if !c.db.lockKeyRead(c.argv[1]) { // expired
		c.addReply(shared.nullbulk)
		return
	}
	set, ok := c.getSetOrReply(c.db.lookupKeyRead(c.argv[1]), shared.nullbulk)
	if !ok || set == nil {
		return
	}

	ele := setTypeRandomElement(set)
	reply, _ := getString(ele)
	c.addReplyBulk(reply)
}

func srandmemberWithCountCommand(c *client) {
	uniq := true
	// Get the count argument
	ll, ok := c.getLongLongOrReply(c.argv[2], nil)
	if !ok {
		return
	} else if ll < 0 {
		// A negative count means: return the same elements multiple times
		// (i.e. don't remove the extracted element after every extraction).
		uniq = false
		ll = -ll
	}
	count := int(ll)

	if !c.db.lockKeyRead(c.argv[1]) { // expired
		c.addReply(shared.emptymultibulk)
		return
	}
	set, ok := c.getSetOrReply(c.db.lookupKeyRead(c.argv[1]), shared.emptymultibulk)
	if !ok || set == nil {
		return
	}

	// If count is zero, serve an empty multibulk ASAP to avoid special
	// cases later.
	if count == 0 {
		c.addReply(shared.emptymultibulk)
		return
	}
	size := setTypeSize(set)

	// CASE 1: The count was negative, so the extraction method is just:
	// "return N random elements" sampling the whole set every time.
	// This case is trivial and can be served without auxiliary data
	// structures.
	if !uniq {
		c.addReplyMultiBulkLen(count)
		for ; count > 0; count-- {
			ele := setTypeRandomElement(set)
			reply, _ := getString(ele)
			c.addReplyBulk(reply)
		}
		return
	}

	// CASE 2:
	// The number of requested elements is greater than the number of
	// elements inside the set: simply return the whole set.
	if count >= size {
		// do not lock key again
		sunionDiffGenericCommand(c, c.argv[1:2], nil, SET_OP_UNION_NOLOCK)
		return
	}

	// For CASE 3 and CASE 4 we need an auxiliary dictionary.
	d := NewDict(count, count) // TODO: use volatile dict

	if count*SRANDMEMBER_SUB_STRATEGY_MUL > size {
		// CASE 3 : The number of elements inside the set is not greater than
		// SRANDMEMBER_SUB_STRATEGY_MUL times the number of requested elements.
		// In this case we create a set from scratch with all the elements, and
		// subtract random elements to reach the requested number of elements.
		// This is done because if the number of requsted elements is just
		// a bit less than the number of elements in the set, the natural
		// approach used into CASE 3 is highly inefficient.

		si := setTypeInitIterator(set)
		for ele := setTypeNext(si); ele != nil; ele = setTypeNext(si) {
			key, _ := getString(ele)
			d.set(key, nil)
		}
		// Remove random elements to reach the right count.
		for ; size > count; size-- {
			de := d.randomKey()
			d.delete(de.key)
		}
	} else {
		// CASE 4: We have a big set compared to the requested number of
		// elements. In this case we can simply get random elements from the set
		// and add to the temporary set, trying to eventually get enough unique
		// elements to reach the specified count.
		for added := 0; added < count; {
			ele := setTypeRandomElement(set)
			key, _ := getString(ele)
			// Try to add the object to the dictionary. If it already exists
			// free it, otherwise increment the number of objects we have
			// in the result dictionary.
			if d.set(key, nil) {
				added++
			}
		}
	}

	// CASE 3 & 4: send the result to the user.
	c.addReplyMultiBulkLen(count)
	di := d.getIterator()
	for de := di.next(); de != nil; de = di.next() {
		c.addReplyBulk(de.key)
	}
}

func sinterCommand(c *client) {
	sinterGenericCommand(c, c.argv[1:], nil)
}

func sinterstoreCommand(c *client) {
	sinterGenericCommand(c, c.argv[2:], c.argv[1])
}

func sinterGenericCommand(c *client, setkeys [][]byte, dstkey []byte) {
	// Lock all touched keys in main dict.
	var keys [][]byte
	var alives []bool = nil
	if dstkey == nil { // read only commands
		keys = setkeys
		alives = c.db.lockKeysRead(keys, 1)
	} else { // write commands.
		// TODO: write locks will be automatically aquired for all keys even we
		// only need read locks for source keys.
		keys = append(setkeys, dstkey)
		c.db.lockKeysWrite(keys, 1)
	}

	sets := make([]interface{}, len(setkeys))
	for i, key := range setkeys {
		if len(alives) > 0 && !alives[i] { // read expired
			return
		}
		if set, ok := c.getSetOrReply(c.db.lookupKeyWrite(key), nil); !ok {
			return
		} else {
			if set == nil {
				if dstkey != nil {
					c.db.delete(dstkey)
					c.addReply(shared.czero)
				} else {
					c.addReply(shared.emptymultibulk)
				}
				return
			} else {
				sets[i] = set
			}
		}
	}

	// Sort sets from the smallest to largest, this will improve our
	// algorithm's performance
	sort.Sort(setslice(sets))

	// The first thing we should output is the total number of elements...
	// since this is a multi-bulk write, but at this stage we don't know
	// the intersection set size, so we use a trick, append an empty object
	// to the output list and save the pointer to later modify it with the
	// right length
	var dstset interface{}
	if dstkey == nil {
		c.addDeferredMultiBulkLength()
	} else {
		dstset = setTypeCreate(nil)
	}

	// Iterate all the elements of the first (smallest) set, and test
	// the element against all the other sets, if at least one set does
	// not include the element it is discarded
	cardinality := 0
	si := setTypeInitIterator(sets[0])
	ele := setTypeNext(si)
	for ele != nil {
		j := 1
		for ; j < len(sets); j++ {
			if sets[j] == sets[0] {
				continue
			}
			if !setTypeIsMember(sets[j], ele) {
				break
			}
		}
		// Only take action when all sets contain the member
		if j == len(sets) {
			if dstkey == nil {
				e, _ := getString(ele)
				c.addReplyBulk(e)
				cardinality++
			} else {
				setTypeAdd(c, dstkey, dstset, ele)
			}
		}
		ele = setTypeNext(si)
	}

	if dstkey != nil {
		c.db.delete(dstkey)
		if setTypeSize(dstset) > 0 {
			c.db.setKey(shadowCopyToPmem(dstkey), dstset)
			c.addReplyLongLong(int64(setTypeSize(dstset)))
		} else {
			c.addReply(shared.czero)
		}
	} else {
		c.setDeferredMultiBulkLength(cardinality)
	}
}

func sunionCommand(c *client) {
	sunionDiffGenericCommand(c, c.argv[1:], nil, SET_OP_UNION)
}

func sunionstoreCommand(c *client) {
	sunionDiffGenericCommand(c, c.argv[2:], c.argv[1], SET_OP_UNION)
}

func sdiffCommand(c *client) {
	sunionDiffGenericCommand(c, c.argv[1:], nil, SET_OP_DIFF)
}

func sdiffstoreCommand(c *client) {
	sunionDiffGenericCommand(c, c.argv[2:], c.argv[1], SET_OP_DIFF)
}

func sunionDiffGenericCommand(c *client, setkeys [][]byte, dstkey []byte, op int) {
	// Lock all touched keys in main dict.
	var keys [][]byte
	var alives []bool = nil
	if op != SET_OP_UNION_NOLOCK {
		if dstkey == nil { // read only commands
			keys = setkeys
			alives = c.db.lockKeysRead(keys, 1)
		} else { // write commands.
			// TODO: write locks will be automatically aquired for all keys even
			// we only need read locks for source keys.
			keys = append(setkeys, dstkey)
			c.db.lockKeysWrite(keys, 1)
		}
	}

	sets := make([]interface{}, len(setkeys))
	for i, key := range setkeys {
		if len(alives) > 0 && !alives[i] { // read expired
			continue
		}
		// TODO: distinguish lookupKeyRead/lookupKeyWrite
		if set, ok := c.getSetOrReply(c.db.lookupKeyWrite(key), nil); !ok {
			return
		} else {
			sets[i] = set
		}
	}

	// Select what DIFF algorithm to use.
	//
	// Algorithm 1 is O(N*M) where N is the size of the element first set
	// and M the total number of sets.
	//
	// Algorithm 2 is O(N) where N is the total number of elements in all
	// the sets.
	//
	// We compute what is the best bet with the current input here.
	diff_algo := 1
	if op == SET_OP_DIFF && sets[0] != nil {
		algo_one_work, algo_two_work := 0, 0
		for j := 0; j < len(sets); j++ {
			if sets[j] == nil {
				continue
			}
			algo_one_work += setTypeSize(sets[0])
			algo_two_work += setTypeSize(sets[j])
		}
		// Algorithm 1 has better constant times and performs less operations
		// if there are elements in common. Give it some advantage.
		algo_one_work /= 2
		if algo_one_work > algo_two_work {
			diff_algo = 2
		}
		if diff_algo == 1 && len(sets) > 1 {
			// With algorithm 1 it is better to order the sets to subtract
			// by decreasing size, so that we are more likely to find
			// duplicated elements ASAP.
			sort.Sort(revsetslice(sets[1:]))
		}
	}

	// We need a temp set object to store our union. If the dstkey
	// is not NULL (that is, we are inside an SUNIONSTORE operation) then
	// this set object will be the resulting object to set into the target key
	dstset := setTypeCreate(nil)
	cardinality := 0

	if op == SET_OP_UNION || op == SET_OP_UNION_NOLOCK {
		// Union is trivial, just add every element of every set to the
		// temporary set.
		for j := 0; j < len(sets); j++ {
			if sets[j] == nil {
				continue
			}
			si := setTypeInitIterator(sets[j])
			ele := setTypeNext(si)
			for ele != nil {
				if setTypeAdd(c, dstkey, dstset, ele) {
					cardinality++
				}
				ele = setTypeNext(si)
			}
		}
	} else if op == SET_OP_DIFF && sets[0] != nil && diff_algo == 1 {
		// DIFF Algorithm 1:
		//
		// We perform the diff by iterating all the elements of the first set,
		// and only adding it to the target set if the element does not exist
		// into all the other sets.
		//
		// This way we perform at max N*M operations, where N is the size of
		// the first set, and M the number of sets.
		si := setTypeInitIterator(sets[0])
		for ele := setTypeNext(si); ele != nil; ele = setTypeNext(si) {
			j := 1
			for ; j < len(sets); j++ {
				if sets[j] == nil {
					continue
				} else if sets[j] == sets[0] {
					break
				} else if setTypeIsMember(sets[j], ele) {
					break
				}
			}
			if j == len(sets) {
				// There is no other set with this element. Add it.
				// should be OK even if dstkey is nil
				setTypeAdd(c, dstkey, dstset, ele)
				cardinality++
			}
		}
	} else if op == SET_OP_DIFF && sets[0] != nil && diff_algo == 2 {
		// DIFF Algorithm 2:
		//
		// Add all the elements of the first set to the auxiliary set.
		// Then remove all the elements of all the next sets from it.
		//
		// This is O(N) where N is the sum of all the elements in every
		// set.
		for j := 0; j < len(sets); j++ {
			if sets[j] == nil {
				continue
			}
			si := setTypeInitIterator(sets[j])
			for ele := setTypeNext(si); ele != nil; ele = setTypeNext(si) {
				if j == 0 {
					setTypeAdd(c, dstkey, dstset, ele)
					cardinality++
				} else {
					if setTypeRemove(c, dstkey, dstset, ele) {
						cardinality--
					}
				}
			}
			// Exit if result set is empty as any additional removal
			// of elements will have no effect.
			if cardinality == 0 {
				break
			}
		}
	}

	// Output the content of the resulting set, if not in STORE mode
	if dstkey == nil {
		c.addReplyMultiBulkLen(cardinality)
		si := setTypeInitIterator(dstset)
		for ele := setTypeNext(si); ele != nil; ele = setTypeNext(si) {
			reply, _ := getString(ele)
			c.addReplyBulk(reply)
		}
	} else {
		// If we have a target key where to store the resulting set
		// create this key with the result set inside
		c.db.delete(dstkey)
		if cardinality > 0 {
			c.db.setKey(shadowCopyToPmem(dstkey), dstset)
			c.addReplyLongLong(int64(cardinality))
		} else {
			c.addReply(shared.czero)
		}
	}
}

func sismemberCommand(c *client) {
	if c.db.lockKeyRead(c.argv[1]) {
		o, ok := c.getSetOrReply(c.db.lookupKeyRead(c.argv[1]), shared.czero)
		if !ok || o == nil {
			return
		} else {
			if setTypeIsMember(o, c.argv[2]) {
				c.addReply(shared.cone)
			} else {
				c.addReply(shared.czero)
			}
		}
	} else { // expired
		c.addReply(shared.czero)
	}
}

func scardCommand(c *client) {
	if c.db.lockKeyRead(c.argv[1]) {
		o, ok := c.getSetOrReply(c.db.lookupKeyRead(c.argv[1]), shared.czero)
		if !ok || o == nil {
			return
		} else {
			c.addReplyLongLong(int64(setTypeSize(o)))
		}
	} else { // expired
		c.addReply(shared.czero)
	}
}

// ============== settype helper functions ====================
func setTypeCreate(val interface{}) interface{} {
	// TODO: create intset or hash based on val.
	return NewDict(4, 4)
}

func setTypeAdd(c *client, skey []byte, subject interface{}, k interface{}) bool {
	switch s := subject.(type) {
	case *dict:
		key, _ := getString(k)
		_, _, _, de := s.find(key)
		if de == nil { // only add if not exist
			s.set(shadowCopyToPmem(key), nil)
			go hashTypeBgResize(c.db, skey)
			return true
		} else {
			return false
		}
	default:
		panic("Unknown set encoding")
	}
}

func setTypeRemove(c *client, skey []byte, subject interface{}, k interface{}) bool {
	switch s := subject.(type) {
	case *dict:
		key, _ := getString(k)
		de := s.delete(key)
		if de == nil {
			return false
		} else {
			go hashTypeBgResize(c.db, skey)
			return true
		}
	default:
		panic("Unknown set encoding")
	}
}

func setTypeIsMember(subject interface{}, k interface{}) bool {
	switch s := subject.(type) {
	case *dict:
		key, _ := getString(k)
		_, _, _, de := s.find(key)
		return de != nil
	default:
		panic("Unknown set encoding")
	}
}

func setTypeRandomElement(subject interface{}) interface{} {
	switch set := subject.(type) {
	case *dict:
		de := set.randomKey()
		if de == nil {
			return nil
		} else {
			return de.key
		}
	default:
		panic("Unknown set encoding")
	}
}

func setTypeSize(subject interface{}) int {
	switch s := subject.(type) {
	case *dict:
		return s.size()
	case nil:
		return 0
	default:
		panic("Unknown set encoding")
	}
}

func setTypeInitIterator(subject interface{}) *setTypeIterator {
	si := new(setTypeIterator)
	si.subject = subject
	switch set := subject.(type) {
	case *dict:
		si.di = set.getIterator()
	default:
		panic("Unknown set encoding")
	}
	return si
}

func setTypeNext(si *setTypeIterator) interface{} {
	switch si.subject.(type) {
	case *dict:
		de := si.di.next()
		if de == nil {
			return nil
		} else {
			return de.key
		}
	default:
		panic("Unknown set encoding")
	}
}

func (sets setslice) Len() int {
	return len(sets)
}

func (sets setslice) Swap(i, j int) {
	sets[i], sets[j] = sets[j], sets[i]
}

func (sets setslice) Less(i, j int) bool {
	return setTypeSize(sets[i]) < setTypeSize(sets[j])
}

func (sets revsetslice) Len() int {
	return len(sets)
}

func (sets revsetslice) Swap(i, j int) {
	sets[i], sets[j] = sets[j], sets[i]
}

func (sets revsetslice) Less(i, j int) bool {
	return setTypeSize(sets[i]) >= setTypeSize(sets[j])
}
