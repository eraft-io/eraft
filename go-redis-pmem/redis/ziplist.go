///////////////////////////////////////////////////////////////////////
// Copyright 2018-2019 VMware, Inc.
// SPDX-License-Identifier: BSD-2-Clause
///////////////////////////////////////////////////////////////////////

package redis

import (
	"bytes"
	"encoding/binary"
	"fmt"
	"math"
	"runtime"
	"strconv"
	"unsafe"

	"github.com/vmware/go-pmem-transaction/transaction"
)

type (
	ziplist struct {
		zltail  int
		entries uint
		data    []byte
	}

	zlentry struct {
		prevlensize, prevlen, lensize, dlen, headersize uint32
		encoding                                        byte
		entry                                           []byte
	}
)

const (
	ZIP_BIG_PREVLEN  uint8 = 254
	ZIP_STR_MASK     uint8 = 0xc0
	ZIP_STR_06B      uint8 = 0 << 6
	ZIP_STR_14B      uint8 = 1 << 6
	ZIP_STR_32B      uint8 = 2 << 6
	ZIP_INT_16B      uint8 = 0xc0 | 0<<4
	ZIP_INT_32B      uint8 = 0xc0 | 1<<4
	ZIP_INT_64B      uint8 = 0xc0 | 2<<4
	ZIP_INT_24B      uint8 = 0xc0 | 3<<4
	ZIP_INT_8B       uint8 = 0xfe
	ZIP_INT_IMM_MIN  uint8 = 0xf1
	ZIP_INT_IMM_MAX  uint8 = 0xfd
	ZIP_INT_IMM_MASK uint8 = 0x0f
)

func ziplistNew() *ziplist {
	return pnew(ziplist)
}

// get val at given offset
func (zl *ziplist) Get(pos int) interface{} {
	if pos >= len(zl.data) || pos < 0 {
		return nil
	}
	var entry zlentry
	entry.set(zl.data[pos:])
	// fmt.Println("Get entry at pos", pos, entry)
	if (entry.encoding & ZIP_STR_MASK) < ZIP_STR_MASK { // string encoding entry
		return zl.data[uint32(pos)+entry.headersize : uint32(pos)+entry.headersize+entry.dlen]
	} else { // int encoding entry
		return zipLoadInteger(zl.data[uint32(pos)+entry.headersize:], entry.encoding)
	}
}

// return offset to use for Get and Next. Traverse back to front if idx is
// negative. Return len(zl.data) if out of range.
func (zl *ziplist) Index(idx int) int {
	pos := 0
	if idx < 0 { // travel backward
		idx = (-idx) - 1
		if len(zl.data) > 0 {
			pos = zl.zltail
			_, prevlen := zipEntryPrevlen(zl.data[pos:])
			for prevlen > 0 && idx > 0 {
				idx--
				pos -= int(prevlen)
				_, prevlen = zipEntryPrevlen(zl.data[pos:])
			}
		}
	} else { // travel forward
		for pos < len(zl.data) && idx > 0 {
			idx--
			pos += int(zipEntrylen(zl.data[pos:]))
		}
	}
	if idx > 0 || pos == len(zl.data) {
		return -1
	} else {
		return pos
	}
}

// compare entry at pos with given val.
func (zl *ziplist) Compare(pos int, val interface{}) bool {
	s, ll, encoding := zipTryEncoding(val)
	return zl.compare(pos, s, ll, encoding)
}

func (zl *ziplist) compare(pos int, s []byte, ll int64, encoding byte) bool {
	e := zl.Get(pos)
	if encoding < ZIP_STR_MASK {
		v, ok := e.([]byte)
		if !ok {
			return false
		} else {
			return bytes.Equal(s, v)
		}
	} else {
		v, ok := e.(int64)
		if !ok {
			return false
		} else {
			return ll == v
		}
	}
}

// find a given val in ziplist and return pos or -1, skip 'skip' entries between
// every comparison.
func (zl *ziplist) Find(val interface{}, skip uint) int {
	skipcnt := uint(0)
	s, ll, encoding := zipTryEncoding(val)
	for pos := 0; pos >= 0; {
		if skipcnt == 0 {
			if zl.compare(pos, s, ll, encoding) {
				return pos
			}
			skipcnt = skip
		} else {
			skipcnt--
		}
		pos = zl.Next(pos)
	}
	return -1
}

// return offset of next entry given current entry offset.
func (zl *ziplist) Next(pos int) int {
	if pos >= len(zl.data) || pos < 0 {
		return -1
	}
	pos += int(zipEntrylen(zl.data[pos:]))
	if pos == len(zl.data) {
		return -1
	} else {
		return pos
	}
}

// return offset of previous entry given current entry offset.
func (zl *ziplist) Prev(pos int) int {
	if pos == 0 {
		return -1
	} else if pos == -1 || pos == len(zl.data) {
		if len(zl.data) == 0 {
			return -1
		} else {
			return zl.zltail
		}
	} else {
		_, prevlen := zipEntryPrevlen(zl.data[pos:])
		return pos - int(prevlen)
	}
}

func (zl *ziplist) Len() int {
	return len(zl.data)
}

// push val to the tail of list.
func (zl *ziplist) Push(val interface{}, head bool) {
	if head {
		zl.insert(0, val)
	} else {
		zl.insert(len(zl.data), val)
	}
}

// delete a single entry at pos
func (zl *ziplist) Delete(pos int) {
	zl.delete(pos, 1)
}

// delete range of entries from pos.
func (zl *ziplist) DeleteRange(idx int, num uint) {
	pos := zl.Index(idx)
	if pos >= 0 {
		zl.delete(pos, num)
	}
}

// append zl2 to zl
func (zl *ziplist) Merge(zl2 *ziplist) {
	if zl == zl2 {
		return // do not allow merge self
	}
	txn("undo") {
	// update entries and new tail position
	zl.entries += zl2.entries
	oldtail := zl.zltail
	zl.zltail = zl2.zltail + len(zl.data)
	// concate data into newdata. TODO: efficient pmem realloc/memmove
	newdata := pmake([]byte, len(zl.data)+len(zl2.data))
	copy(newdata, zl.data)
	copy(newdata[len(zl.data):], zl2.data)
	// cascade update
	tailshift := 0
	newdata, tailshift = cascadeUpdate(newdata, oldtail)
	zl.zltail += tailshift
	zl.data = newdata
	}
}

func (zl *ziplist) insert(pos int, val interface{}) {
	// construct new entry in tmp volatile slice
	prevlensize, prevlen := zl.prevlen(pos)
	// fmt.Println("insert to", pos, "prev len", prevlensize, prevlen)
	// give enough room for two prevlen and encoding
	entry := make([]byte, prevlensize+10)
	// store prevlen
	if prevlensize == 1 {
		entry[0] = uint8(prevlen)
	} else {
		entry[0] = ZIP_BIG_PREVLEN
		binary.LittleEndian.PutUint32(entry[1:], prevlen)
	}
	// try to encod val
	s, ll, encoding := zipTryEncoding(val)
	// store encoding and len
	encodesize := zipStoreEntryEncoding(entry[prevlensize:], encoding, len(s))
	// store the val
	if s != nil { // string encoding, direct copy
		entry = entry[:prevlensize+encodesize] // trim unused space
		entry = append(entry, s...)
	} else { // int encoding
		llsize := zipSaveInteger(entry[prevlensize+encodesize:], ll, encoding)
		entry = entry[:prevlensize+encodesize+llsize] // trim unused space
	}
	// fmt.Println("newentry", len(entry), entry)

	// insert entry into zl
	txn("undo") {
	// TODO: implement persistent append/realloc/memmove. Currently always copy to newly allocated slice
	newdata := pmake([]byte, len(zl.data)+len(entry))
	tailShift := 0
	copy(newdata, zl.data[0:pos])
	copy(newdata[pos:], entry)
	copy(newdata[pos+len(entry):], zl.data[pos:])
	if pos == len(zl.data) { // insert to the tail
		zl.zltail = len(zl.data)
		// fmt.Println("insert to tail, new tail", zl.zltail)
	} else { // insert into middle, need to update prevlen of following entries
		// cascade update following entries
		newdata, tailShift = cascadeUpdate(newdata, pos)
		zl.zltail += len(entry) + tailShift
		// fmt.Println("insert to middle, new tail", zl.zltail)
	}
	runtime.FlushRange(unsafe.Pointer(&newdata[0]), uintptr(len(newdata)))
	zl.data = newdata
	zl.entries++
	}
	// fmt.Println("insert finish, new entries, len", zl.entries, len(zl.data), zl.data)
}

func (zl *ziplist) delete(pos int, num uint) {
	deleted := uint(0)
	end := pos
	for deleted < num && end >= 0 {
		end = zl.Next(end)
		deleted++
	}
	if deleted > 0 {
		txn("undo") {
		zl.entries -= deleted
		prevlensize, prevlen := zipEntryPrevlen(zl.data[pos:])
		if end == -1 {
			// delete entire tails, directly cut slice in this case to save copying cost.
			// TODO: downside is that the cutted tail will not be garbage collected.
			zl.data = zl.data[:pos]
			zl.zltail = pos - int(prevlen)
		} else {
			// use prevlen of first deleted entry to replace the prevlen of the new next entry.
			oldprevlensize, _ := zipEntryPrevlen(zl.data[end:])
			if zl.zltail == end {
				// need to consider the prevlen size diff if change the prevlen field of tail
				zl.zltail = pos
			} else {
				zl.zltail -= (end - pos + int(oldprevlensize) - int(prevlensize))
			}
			pos += int(prevlensize)
			end += int(oldprevlensize)
			newdata := pmake([]byte, len(zl.data)-(end-pos))
			copy(newdata, zl.data[:pos])
			copy(newdata[pos:], zl.data[end:])

			// cascade update prevlen
			newdata, tailShift := cascadeUpdate(newdata, pos-int(prevlensize))
			runtime.FlushRange(unsafe.Pointer(&newdata[0]), uintptr(len(newdata)))
			zl.data = newdata
			zl.zltail += tailShift
		}
		}
	}
}

func cascadeUpdate(data []byte, pos int) ([]byte, int) {
	tailShift := 0
	for pos < len(data) {
		// get curr entry size
		currsize := zipEntrylen(data[pos:])
		pos += int(currsize)
		if pos == len(data) { // reach end of list
			break
		}
		// get prevlen field of next entry
		prevlensize, prevlen := zipEntryPrevlen(data[pos:])
		if prevlen == currsize { // no size change
			break
		} else if currsize < uint32(ZIP_BIG_PREVLEN) || prevlensize == 5 {
			// enough space to hold currsize
			if prevlensize == 1 {
				data[pos] = uint8(currsize)
			} else {
				binary.LittleEndian.PutUint32(data[pos+1:], currsize)
			}
		} else { // need more room to hold currsize
			// TODO: implement persistent append/realloc/memmove
			if pos+int(zipEntrylen(data[pos:])) < len(data) {
				// next entry is not tail
				tailShift += 4
			}
			newdata := pmake([]byte, len(data)+4)
			copy(newdata, data[:pos])
			copy(newdata[pos+5:], data[pos+1:])
			newdata[pos] = ZIP_BIG_PREVLEN
			binary.LittleEndian.PutUint32(newdata[pos+1:], currsize)
			data = newdata
		}
	}
	return data, tailShift
}

// get information of an entry and put into zlentry struct.
func (ze *zlentry) set(entry []byte) {
	ze.prevlensize, ze.prevlen = zipEntryPrevlen(entry)
	ze.lensize, ze.dlen = zipEntryDatalen(entry[ze.prevlensize:])
	ze.headersize = ze.prevlensize + ze.lensize
	ze.encoding = entry[ze.prevlensize]
	ze.entry = entry
}

// get size to hold prev len and prev entry len of entry at given position.
func (zl *ziplist) prevlen(pos int) (uint32, uint32) {
	if len(zl.data) == 0 || pos == 0 { // insert to head or an empty list
		return 1, 0
	} else if pos < len(zl.data) { // get prev entry len from encoding of curr entry
		return zipEntryPrevlen(zl.data[pos:])
	} else { // insert to tail, calculate prev entry len of old tail entry
		prevlen := zipEntrylen(zl.data[zl.zltail:])
		if prevlen < uint32(ZIP_BIG_PREVLEN) {
			return 1, prevlen
		} else {
			return 5, prevlen
		}
	}
}

func zipEntryPrevlen(entry []byte) (uint32, uint32) {
	if entry[0] < ZIP_BIG_PREVLEN {
		return 1, uint32(entry[0])
	} else {
		return 5, binary.LittleEndian.Uint32(entry[1:])
	}
}

// len of an entry, including [prevlen, encoding, len, data]
func zipEntrylen(entry []byte) uint32 {
	var prevlensize uint32
	// Do not need to decode prelen in this case.
	if entry[0] < ZIP_BIG_PREVLEN {
		prevlensize = 1
	} else {
		prevlensize = 5
	}
	lensize, size := zipEntryDatalen(entry[prevlensize:])
	return prevlensize + lensize + size
}

// len of entry data, return size of (encoding+len) and size of (data)
func zipEntryDatalen(entry []byte) (uint32, uint32) {
	encoding := entry[0]
	// fmt.Println("Get data len, encoding:", encoding)
	if encoding < ZIP_STR_MASK {
		encoding &= ZIP_STR_MASK
		if encoding == ZIP_STR_06B {
			// fmt.Println("str 6B", uint32(entry[0] & 0x3f))
			return 1, uint32(entry[0] & 0x3f)
		} else if encoding == ZIP_STR_14B {
			tmp := make([]byte, 2)
			tmp[0] = entry[0] & 0x3f
			tmp[1] = entry[1]
			// fmt.Println("str 14B", uint32(binary.BigEndian.Uint16(tmp)))
			return 2, uint32(binary.BigEndian.Uint16(tmp))
		} else if encoding == ZIP_STR_32B {
			// fmt.Println("str 32B", binary.BigEndian.Uint32(entry[1:]))
			return 5, binary.BigEndian.Uint32(entry[1:])
		} else {
			panic("Invalid string encoding")
		}
	} else {
		switch encoding {
		case ZIP_INT_8B:
			return 1, 1
		case ZIP_INT_16B:
			return 1, 2
		case ZIP_INT_24B:
			return 1, 3
		case ZIP_INT_32B:
			return 1, 4
		case ZIP_INT_64B:
			return 1, 8
		}
		if encoding >= ZIP_INT_IMM_MIN && encoding <= ZIP_INT_IMM_MAX {
			return 1, 0
		}
		panic("Invalid integer encoding")
	}
	return 0, 0
}

// determine encoding of val by its type and length
func zipTryEncoding(val interface{}) ([]byte, int64, byte) {
	var value int64
	switch v := val.(type) {
	case []byte:
		if len(v) >= 32 || len(v) == 0 {
			return v, 0, 0
		} else {
			if ll, err := strconv.ParseInt(string(v), 10, 64); err == nil {
				value = ll
			} else {
				return v, 0, 0
			}
		}
	case int64:
		value = v
	default:
		panic("Invalid ziplist value")
	}

	if value >= 0 && value <= 12 {
		return nil, value, ZIP_INT_IMM_MIN + uint8(value)
	} else if value >= math.MinInt8 && value <= math.MaxInt8 {
		return nil, value, ZIP_INT_8B
	} else if value >= math.MinInt16 && value <= math.MaxInt16 {
		return nil, value, ZIP_INT_16B
	} else if value >= math.MinInt32 && value <= math.MaxInt32 {
		return nil, value, ZIP_INT_32B
	} else {
		return nil, value, ZIP_INT_64B
	}
}

// store [encoding, len] into entry
func zipStoreEntryEncoding(v []byte, encoding byte, size int) uint32 {
	if (encoding & ZIP_STR_MASK) < ZIP_STR_MASK { // str encoding
		if size <= 0x3f {
			v[0] = ZIP_STR_06B | uint8(size)
			return 1
		} else if size <= 0x3fff {
			binary.BigEndian.PutUint16(v, uint16(size))
			v[0] = (v[0] & 0x3f) | ZIP_STR_14B
			return 2
		} else {
			v[0] = ZIP_STR_32B
			binary.BigEndian.PutUint32(v[1:], uint32(size))
			return 5
		}
	} else { // integer encoding
		v[0] = encoding
	}
	return 1
}

// store interger data into entry.
func zipSaveInteger(v []byte, ll int64, encoding byte) uint32 {
	if encoding == ZIP_INT_8B {
		v[0] = byte(ll)
		return 1
	} else if encoding == ZIP_INT_16B {
		binary.LittleEndian.PutUint16(v, uint16(ll))
		return 2
	} else if encoding == ZIP_INT_32B {
		binary.LittleEndian.PutUint32(v, uint32(ll))
		return 4
	} else if encoding == ZIP_INT_64B {
		binary.LittleEndian.PutUint64(v, uint64(ll))
		return 8
	} else if encoding >= ZIP_INT_IMM_MIN && encoding <= ZIP_INT_IMM_MAX {
		return 0
	} else {
		panic("Save unknown interger encoding.")
	}
}

// load interger data from entry.
func zipLoadInteger(v []byte, encoding byte) int64 {
	var ret int64
	if encoding == ZIP_INT_8B {
		ret = int64(int8(v[0]))
	} else if encoding == ZIP_INT_16B {
		ret = int64(int16(binary.LittleEndian.Uint16(v)))
	} else if encoding == ZIP_INT_32B {
		ret = int64(int32(binary.LittleEndian.Uint32(v)))
	} else if encoding == ZIP_INT_64B {
		ret = int64(binary.LittleEndian.Uint64(v))
	} else if encoding >= ZIP_INT_IMM_MIN && encoding <= ZIP_INT_IMM_MAX {
		ret = int64(encoding&ZIP_INT_IMM_MASK) - 1
	} else {
		panic("Save unknown interger encoding.")
	}
	return ret
}

func (zl *ziplist) deepCopy() *ziplist {
	newzl := ziplistNew()
	newdata := pmake([]byte, zl.Len())
	copy(newdata, zl.data)
	runtime.FlushRange(unsafe.Pointer(&newdata[0]), uintptr(len(newdata)))
	txn("undo") {
	newzl.entries = zl.entries
	newzl.zltail = zl.zltail
	newzl.data = newdata
	}
	return newzl
}

func (zl *ziplist) print() {
	fmt.Print("[", zl.entries, " ", zl.zltail, " ", zl.Len(), ": ")
	for i := 0; i < int(zl.entries); i++ {
		p := zl.Index(i)
		v := zl.Get(p)
		switch s := v.(type) {
		case []byte:
			fmt.Print(p, " ", string(s), ", ")
		default:
			fmt.Print(p, " ", s, ", ")
		}
	}
	fmt.Print("]\n")
}

func (zl *ziplist) verify() bool {
	if zl.Len() == 0 {
		if zl.entries != 0 || zl.zltail != 0 {
			println("zl size mismatch with data len!")
			return false
		}
		return true
	}
	size := uint(0)
	pos := 0
	for pos >= 0 {
		size++
		pos = zl.Next(pos)
	}
	if size != zl.entries {
		println("zl size mismatch when iterate forward!")
		return false
	}
	size = 0
	pos = zl.Len()
	for pos > 0 {
		size++
		pos = zl.Prev(pos)
	}
	if size != zl.entries {
		println("zl size mismatch when iterate backward!")
		return false
	}
	return true
}
