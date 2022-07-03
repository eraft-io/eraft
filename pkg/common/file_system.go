package common

import (
	"crypto/md5"
	"fmt"
	"io/ioutil"
	"os"
)

//
// CalFileCheckSum
// calculate a file md5 checksum
//
func CalFileCheckSumMD5(path string) [16]byte {
	var md5_check_sum [16]byte
	file, err_open := os.Open(path)
	if err_open != nil {
		fmt.Println("Open file fail", err_open)
		return md5_check_sum
	}
	defer file.Close()
	content, err_read := ioutil.ReadAll(file)
	if err_read != nil {
		fmt.Println("Read file fail", err_read)
		return md5_check_sum
	}
	//var md5_check_sum = md5.Sum(content)
	md5_check_sum = md5.Sum(content)
	return md5_check_sum
}
