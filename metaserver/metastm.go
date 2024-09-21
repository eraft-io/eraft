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

package metaserver

import (
	"encoding/json"
	"strconv"

	"github.com/eraft-io/eraft/common"
	"github.com/eraft-io/eraft/logger"
	"github.com/eraft-io/eraft/storage"
)

const CF_PREFIX = "CF_"

const CUR_VERSION_KEY = "CUR_CONF_VERSION"

type ConfigStm interface {
	Join(groups map[int][]string) error
	Leave(gids []int) error
	Move(bucketID, gID int) error
	Query(num int) (Config, error)
}

type MemConfigStm struct {
	dbEng          storage.KvStore
	curConfVersion int
}

func NewMemConfigStm(dbEng storage.KvStore) *MemConfigStm {
	// check if has default conf
	_, err := dbEng.Get(CF_PREFIX + strconv.Itoa(0))
	confStm := &MemConfigStm{dbEng: dbEng, curConfVersion: 0}
	if err != nil {
		defaultConfig := DefaultConfig()
		defaultConfigBytes, err := json.Marshal(defaultConfig)
		if err != nil {
			panic(err)
		}
		// init conf
		logger.ELogger().Sugar().Debugf("init conf -> " + string(defaultConfigBytes))
		if err := confStm.dbEng.Put(CF_PREFIX+strconv.Itoa(0), string(defaultConfigBytes)); err != nil {
			panic(err)
		}
		if err := confStm.dbEng.Put(CUR_VERSION_KEY, strconv.Itoa(confStm.curConfVersion)); err != nil {
			panic(err)
		}
		return confStm
	}
	version_str, err := dbEng.Get(CUR_VERSION_KEY)
	if err != nil {
		panic(err)
	}
	version_int, _ := strconv.Atoi(version_str)
	confStm.curConfVersion = version_int
	return confStm
}

func (cfStm *MemConfigStm) Join(groups map[int][]string) error {
	confBytes, err := cfStm.dbEng.Get(CF_PREFIX + strconv.Itoa(cfStm.curConfVersion))
	if err != nil {
		return err
	}
	lastConf := &Config{}
	json.Unmarshal([]byte(confBytes), lastConf)
	newConfig := Config{cfStm.curConfVersion + 1, lastConf.Buckets, deepCopy(lastConf.Groups)}
	for gid, servers := range groups {
		if _, ok := newConfig.Groups[gid]; !ok {
			newSvrs := make([]string, len(servers))
			copy(newSvrs, servers)
			newConfig.Groups[gid] = newSvrs
		}
	}
	s2g := newConfig.GetGroup2Buckets()
	var new_buckets [common.NBuckets]int
	for gid, buckets := range s2g {
		for _, bid := range buckets {
			new_buckets[bid] = gid
		}
	}
	newConfig.Buckets = new_buckets
	newConfigBytes, _ := json.Marshal(newConfig)
	cfStm.dbEng.Put(CUR_VERSION_KEY, strconv.Itoa(cfStm.curConfVersion+1))
	cfStm.dbEng.Put(CF_PREFIX+strconv.Itoa(cfStm.curConfVersion+1), string(newConfigBytes))
	cfStm.curConfVersion += 1
	return nil
}

func (cfStm *MemConfigStm) Leave(gids []int) error {
	confBytes, err := cfStm.dbEng.Get(CF_PREFIX + strconv.Itoa(cfStm.curConfVersion))
	if err != nil {
		return err
	}
	lastConf := &Config{}
	json.Unmarshal([]byte(confBytes), lastConf)
	newConf := Config{cfStm.curConfVersion + 1, lastConf.Buckets, deepCopy(lastConf.Groups)}
	for _, gid := range gids {
		delete(newConf.Groups, gid)
	}
	var newBuckets [common.NBuckets]int
	newConf.Buckets = newBuckets
	newConfigBytes, _ := json.Marshal(newConf)
	cfStm.dbEng.Put(CUR_VERSION_KEY, strconv.Itoa(cfStm.curConfVersion+1))
	cfStm.dbEng.Put(CF_PREFIX+strconv.Itoa(cfStm.curConfVersion+1), string(newConfigBytes))
	cfStm.curConfVersion += 1
	return nil
}

func (cfStm *MemConfigStm) Move(bid, gid int) error {
	confBytes, err := cfStm.dbEng.Get(CF_PREFIX + strconv.Itoa(cfStm.curConfVersion))
	if err != nil {
		return err
	}
	lastConf := &Config{}
	json.Unmarshal([]byte(confBytes), lastConf)
	newConf := Config{cfStm.curConfVersion + 1, lastConf.Buckets, deepCopy(lastConf.Groups)}
	newConf.Buckets[bid] = gid
	newConfigBytes, _ := json.Marshal(newConf)
	cfStm.dbEng.Put(CUR_VERSION_KEY, strconv.Itoa(cfStm.curConfVersion+1))
	cfStm.dbEng.Put(CF_PREFIX+strconv.Itoa(cfStm.curConfVersion+1), string(newConfigBytes))
	cfStm.curConfVersion += 1
	return nil
}

func (cfStm *MemConfigStm) Query(version int) (Config, error) {
	if version < 0 || version >= cfStm.curConfVersion {
		lastConf := &Config{}
		logger.ELogger().Sugar().Debugf("query cur version -> " + strconv.Itoa(cfStm.curConfVersion))
		confBytes, err := cfStm.dbEng.Get(CF_PREFIX + strconv.Itoa(cfStm.curConfVersion))
		if err != nil {
			return DefaultConfig(), err
		}
		json.Unmarshal([]byte(confBytes), lastConf)
		return *lastConf, nil
	}
	confBytes, err := cfStm.dbEng.Get(CF_PREFIX + strconv.Itoa(version))
	if err != nil {
		return DefaultConfig(), err
	}
	specConf := &Config{}
	json.Unmarshal([]byte(confBytes), specConf)
	return *specConf, nil
}
