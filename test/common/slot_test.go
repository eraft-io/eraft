// Copyright [2022] [WellWood] [wellwood-x@googlegroups.com]

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

// 	http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package common

import (
	"math/rand"
	"strings"
	"testing"
	"time"

	cm "github.com/eraft-io/eraft/pkg/common"
)

func getRandstring(length int) string {
	if length < 1 {
		return ""
	}
	char := "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
	charArr := strings.Split(char, "")
	charlen := len(charArr)
	ran := rand.New(rand.NewSource(time.Now().Unix()))

	var rchar string = ""
	for i := 1; i <= length; i++ {
		rchar = rchar + charArr[ran.Intn(charlen)]
	}
	return rchar
}

func TestSlot(t *testing.T) {
	var sum_all uint64
	for i := 1; i < 5000; i++ {
		var testString = getRandstring(i)
		//t.Log(testString)
		var xhh3res = cm.StrToSlot(testString)
		sum_all += xhh3res
	}
	t.Log(sum_all / 5000)
}
