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

package storage_eng

//
// If you want to contribute a new engine implementation, you need to implement these interfaces
//
type KvStore interface {
	Put(string, string) error
	Get(string) (string, error)
	Delete(string) error
	DumpPrefixKey(string) (map[string]string, error)
	PutBytesKv(k []byte, v []byte) error
	DeleteBytesK(k []byte) error
	GetBytesValue(k []byte) ([]byte, error)
	SeekPrefixLast(prefix string) ([]byte, []byte, error)
	SeekPrefixFirst(prefix string) ([]byte, []byte, error)
	DelPrefixKeys(prefix string) error
}
