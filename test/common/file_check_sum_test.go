package common

import (
	cm "github.com/eraft-io/eraft/pkg/common"
	"testing"
)

func TestCheckSUm(t *testing.T) {
	t.Log(cm.CalFileCheckSumMD5("/Users/luooo/Desktop/CheckSumTest/a.txt"))
}

//[162 143 11 146 76 170 68 124 156 86 132 161 245 48 68 236]
