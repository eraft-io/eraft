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
	Move(bucket_id, gid int) error
	Query(num int) (Config, error)
}

type MemConfigStm struct {
	dbEng          storage.KvStore
	curConfVersion int
}

func NewMemConfigStm(dbEng storage.KvStore) *MemConfigStm {
	// check if has default conf
	_, err := dbEng.Get(CF_PREFIX + strconv.Itoa(0))
	conf_stm := &MemConfigStm{dbEng: dbEng, curConfVersion: 0}
	if err != nil {
		defaultConfig := DefaultConfig()
		defaultConfigBytes, err := json.Marshal(defaultConfig)
		if err != nil {
			panic(err)
		}
		// init conf
		logger.ELogger().Sugar().Debugf("init conf -> " + string(defaultConfigBytes))
		if err := conf_stm.dbEng.Put(CF_PREFIX+strconv.Itoa(0), string(defaultConfigBytes)); err != nil {
			panic(err)
		}
		if err := conf_stm.dbEng.Put(CUR_VERSION_KEY, strconv.Itoa(conf_stm.curConfVersion)); err != nil {
			panic(err)
		}
		return conf_stm
	}
	version_str, err := dbEng.Get(CUR_VERSION_KEY)
	if err != nil {
		panic(err)
	}
	version_int, _ := strconv.Atoi(version_str)
	conf_stm.curConfVersion = version_int
	return conf_stm
}

func (cfStm *MemConfigStm) Join(groups map[int][]string) error {
	conf_bytes, err := cfStm.dbEng.Get(CF_PREFIX + strconv.Itoa(cfStm.curConfVersion))
	if err != nil {
		return err
	}
	last_conf := &Config{}
	json.Unmarshal([]byte(conf_bytes), last_conf)
	new_config := Config{cfStm.curConfVersion + 1, last_conf.Buckets, deepCopy(last_conf.Groups)}
	for gid, servers := range groups {
		if _, ok := new_config.Groups[gid]; !ok {
			newSvrs := make([]string, len(servers))
			copy(newSvrs, servers)
			new_config.Groups[gid] = newSvrs
		}
	}
	s2g := new_config.GetGroup2Buckets()
	var new_buckets [common.NBuckets]int
	for gid, buckets := range s2g {
		for _, bid := range buckets {
			new_buckets[bid] = gid
		}
	}
	new_config.Buckets = new_buckets
	new_config_bytes, _ := json.Marshal(new_config)
	cfStm.dbEng.Put(CUR_VERSION_KEY, strconv.Itoa(cfStm.curConfVersion+1))
	cfStm.dbEng.Put(CF_PREFIX+strconv.Itoa(cfStm.curConfVersion+1), string(new_config_bytes))
	cfStm.curConfVersion += 1
	return nil
}

func (cfStm *MemConfigStm) Leave(gids []int) error {
	conf_bytes, err := cfStm.dbEng.Get(CF_PREFIX + strconv.Itoa(cfStm.curConfVersion))
	if err != nil {
		return err
	}
	last_conf := &Config{}
	json.Unmarshal([]byte(conf_bytes), last_conf)
	new_conf := Config{cfStm.curConfVersion + 1, last_conf.Buckets, deepCopy(last_conf.Groups)}
	for _, gid := range gids {
		delete(new_conf.Groups, gid)
	}
	var newBuckets [common.NBuckets]int
	new_conf.Buckets = newBuckets
	new_config_bytes, _ := json.Marshal(new_conf)
	cfStm.dbEng.Put(CUR_VERSION_KEY, strconv.Itoa(cfStm.curConfVersion+1))
	cfStm.dbEng.Put(CF_PREFIX+strconv.Itoa(cfStm.curConfVersion+1), string(new_config_bytes))
	cfStm.curConfVersion += 1
	return nil
}

func (cfStm *MemConfigStm) Move(bid, gid int) error {
	conf_bytes, err := cfStm.dbEng.Get(CF_PREFIX + strconv.Itoa(cfStm.curConfVersion))
	if err != nil {
		return err
	}
	last_conf := &Config{}
	json.Unmarshal([]byte(conf_bytes), last_conf)
	new_conf := Config{cfStm.curConfVersion + 1, last_conf.Buckets, deepCopy(last_conf.Groups)}
	new_conf.Buckets[bid] = gid
	new_config_bytes, _ := json.Marshal(new_conf)
	cfStm.dbEng.Put(CUR_VERSION_KEY, strconv.Itoa(cfStm.curConfVersion+1))
	cfStm.dbEng.Put(CF_PREFIX+strconv.Itoa(cfStm.curConfVersion+1), string(new_config_bytes))
	cfStm.curConfVersion += 1
	return nil
}

func (cfStm *MemConfigStm) Query(version int) (Config, error) {
	if version < 0 || version >= cfStm.curConfVersion {
		last_conf := &Config{}
		logger.ELogger().Sugar().Debugf("query cur version -> " + strconv.Itoa(cfStm.curConfVersion))
		confBytes, err := cfStm.dbEng.Get(CF_PREFIX + strconv.Itoa(cfStm.curConfVersion))
		if err != nil {
			return DefaultConfig(), err
		}
		json.Unmarshal([]byte(confBytes), last_conf)
		return *last_conf, nil
	}
	conf_bytes, err := cfStm.dbEng.Get(CF_PREFIX + strconv.Itoa(version))
	if err != nil {
		return DefaultConfig(), err
	}
	spec_conf := &Config{}
	json.Unmarshal([]byte(conf_bytes), spec_conf)
	return *spec_conf, nil
}
