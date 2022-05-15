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

package configserver

import (
	"time"

	"github.com/eraft-io/mit6.824lab2product/common"
)

type Config struct {
	Version int
	Buckets [common.NBuckets]int
	Groups  map[int][]string
}

func DefaultConfig() Config {
	return Config{Groups: make(map[int][]string)}
}

func (cf *Config) GetGroup2Buckets() map[int][]int {
	s2g := make(map[int][]int)
	for gid := range cf.Groups {
		s2g[gid] = make([]int, 0)
	}
	for bid, gid := range cf.Buckets {
		s2g[gid] = append(s2g[gid], bid)
	}
	return s2g
}

const ExecTimeout = 3 * time.Second

func deepCopy(groups map[int][]string) map[int][]string {
	newG := make(map[int][]string)
	for gid, severs := range groups {
		newSvrs := make([]string, len(severs))
		copy(newSvrs, severs)
		newG[gid] = newSvrs
	}
	return newG
}
