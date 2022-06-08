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

package kvserver

import "errors"

type StateMachine interface {
	Get(key string) (string, error)
	Put(key, value string) error
	Append(key, value string) error
}

type MemKV struct {
	KV map[string]string
}

func NewMemKV() *MemKV {
	return &MemKV{make(map[string]string)}
}

func (memKv *MemKV) Get(key string) (string, error) {
	if v, ok := memKv.KV[key]; ok {
		return v, nil
	}
	return "", errors.New("KeyNotFound")
}

func (memKv *MemKV) Put(key, value string) error {
	memKv.KV[key] = value
	return nil
}

func (memKv *MemKV) Append(key, value string) error {
	memKv.KV[key] += value
	return nil
}
