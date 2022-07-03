package common

import (
	cm "github.com/eraft-io/eraft/pkg/common"
	"math/rand"
	"strings"
	"testing"
	"time"
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
