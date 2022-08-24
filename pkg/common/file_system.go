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
	"crypto/md5"
	"fmt"
	"io/fs"
	"io/ioutil"
	"os"
	"path/filepath"

	"github.com/eraft-io/eraft/pkg/log"
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
	md5_check_sum = md5.Sum(content)
	return md5_check_sum
}

// ReadFilesMetaInDir ...
func ReadFileMetaInDir(path string) ([]string, int64, error) {
	files, err := ioutil.ReadDir(path)
	if err != nil {
		return []string{}, 0, err
	}
	fileNames := []string{}
	var totalSize int64
	for _, file := range files {
		fileNames = append(fileNames, file.Name())
		totalSize += file.Size()
	}
	return fileNames, totalSize, nil
}

// get total file size in a dir
func GetTotalFileSizeInDir(path string) int64 {
	var totalSize int64
	err := filepath.Walk(path, func(path string, info fs.FileInfo, err error) error {
		if err != nil {
			log.MainLogger.Error().Msgf("err %s", err.Error())
			return nil
		}
		if !info.IsDir() {
			totalSize += info.Size()
		}
		return nil
	})
	if err != nil {
		log.MainLogger.Error().Msgf("err %s", err.Error())
		return -1
	}
	return totalSize
}
