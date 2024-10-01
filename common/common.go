//
// MIT License

// Copyright (c) 2022 eraft dev group

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

package common

import (
	"hash/crc32"
	"io/ioutil"
	"os"
	"path"
)

const NBuckets = 10

const UnUsedTid = 999

const (
	ErrCodeNoErr int64 = iota
	ErrCodeWrongGroup
	ErrCodeWrongLeader
	ErrCodeExecTimeout
)

func Key2BucketID(key string) int {
	return CRC32KeyHash(key, NBuckets)
}

func CRC32KeyHash(k string, base int) int {
	bucketID := 0
	crc32q := crc32.MakeTable(0xD5828281)
	sum := crc32.Checksum([]byte(k), crc32q)
	bucketID = int(sum) % NBuckets
	return bucketID
}

func Int64ArrToIntArr(in []int64) []int {
	out := []int{}
	for _, item := range in {
		out = append(out, int(item))
	}
	return out
}

func RemoveDir(in string) {
	dir, _ := ioutil.ReadDir(in)
	for _, d := range dir {
		os.RemoveAll(path.Join([]string{in, d.Name()}...))
	}
}
