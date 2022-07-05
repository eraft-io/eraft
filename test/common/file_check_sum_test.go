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
	"testing"

	cm "github.com/eraft-io/eraft/pkg/common"
)

func TestCheckSUm(t *testing.T) {
	t.Log(cm.CalFileCheckSumMD5("/Users/luooo/Desktop/CheckSumTest/a.txt"))
}

//[162 143 11 146 76 170 68 124 156 86 132 161 245 48 68 236]
