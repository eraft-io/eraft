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

const CfPrefix = "CF_"

const CurVersionKey = "CURV"

type ConfigStm interface {
	Join(groups map[int][]string) error
	Leave(gids []int) error
	Move(bucketID, gID int) error
	Query(num int) (Config, error)
}

type PersistentConfigStm struct {
	dbEng          storage.KvStore
	curConfVersion int
}

func NewMemConfigStm(dbEng storage.KvStore) *PersistentConfigStm {
	// check if it has default conf
	_, err := dbEng.Get(CfPrefix + strconv.Itoa(0))
	confStm := &PersistentConfigStm{dbEng: dbEng, curConfVersion: 0}
	if err != nil {
		//write init conf to meta db
		defaultConfig := DefaultConfig()
		defaultConfigBytes, err := json.Marshal(defaultConfig)
		if err != nil {
			panic(err)
		}
		logger.ELogger().Sugar().Debugf("init conf -> " + string(defaultConfigBytes))
		if err := confStm.dbEng.Put(CfPrefix+strconv.Itoa(0), string(defaultConfigBytes)); err != nil {
			panic(err)
		}
		if err := confStm.dbEng.Put(CurVersionKey, strconv.Itoa(confStm.curConfVersion)); err != nil {
			panic(err)
		}
		return confStm
	}
	// not first init, get current config version
	versionStr, err := dbEng.Get(CurVersionKey)
	if err != nil {
		panic(err)
	}
	versionInt, err := strconv.Atoi(versionStr)
	if err != nil {
		panic(err)
	}
	confStm.curConfVersion = versionInt
	return confStm
}

func (cfStm *PersistentConfigStm) GetLastConfig() (Config, error) {
	lastConf := &Config{}
	confBytes, err := cfStm.dbEng.Get(CfPrefix + strconv.Itoa(cfStm.curConfVersion))
	if err != nil {
		return DefaultConfig(), err
	}
	json.Unmarshal([]byte(confBytes), lastConf)
	return *lastConf, nil
}

func (cfStm *PersistentConfigStm) Join(groups map[int][]string) error {
	// get last config from db
	lastConf, err := cfStm.GetLastConfig()
	if err != nil {
		return err
	}
	// update config with new groups
	nextConfVersion := cfStm.curConfVersion + 1
	newConfig := Config{nextConfVersion, lastConf.Buckets, deepCopy(lastConf.Groups)}
	for gid, servers := range groups {
		if _, ok := newConfig.Groups[gid]; !ok {
			newSvrs := make([]string, len(servers))
			copy(newSvrs, servers)
			newConfig.Groups[gid] = newSvrs
		}
	}
	s2g := newConfig.GetGroup2Buckets()
	var newBuckets [common.NBuckets]int
	for gid, buckets := range s2g {
		for _, bid := range buckets {
			newBuckets[bid] = gid
		}
	}
	newConfig.Buckets = newBuckets
	newConfigBytes, err := json.Marshal(newConfig)
	if err != nil {
		return err
	}
	// write version and config to db
	cfStm.dbEng.Put(CurVersionKey, strconv.Itoa(nextConfVersion))
	cfStm.dbEng.Put(CfPrefix+strconv.Itoa(nextConfVersion), string(newConfigBytes))
	cfStm.curConfVersion = nextConfVersion
	return nil
}

func (cfStm *PersistentConfigStm) Leave(gids []int) error {
	// get last config from db
	lastConf, err := cfStm.GetLastConfig()
	if err != nil {
		return err
	}
	// update config with removed groups
	nextConfVersion := cfStm.curConfVersion + 1
	newConf := Config{nextConfVersion, lastConf.Buckets, deepCopy(lastConf.Groups)}
	for _, gid := range gids {
		delete(newConf.Groups, gid)
	}
	var newBuckets [common.NBuckets]int
	newConf.Buckets = newBuckets
	newConfigBytes, _ := json.Marshal(newConf)
	cfStm.dbEng.Put(CurVersionKey, strconv.Itoa(nextConfVersion))
	cfStm.dbEng.Put(CfPrefix+strconv.Itoa(nextConfVersion), string(newConfigBytes))
	cfStm.curConfVersion = nextConfVersion
	return nil
}

func (cfStm *PersistentConfigStm) Move(bid, gid int) error {
	// get last config from db
	lastConf, err := cfStm.GetLastConfig()
	if err != nil {
		return err
	}
	// update config with moved bucket
	nextConfVersion := cfStm.curConfVersion + 1
	newConf := Config{nextConfVersion, lastConf.Buckets, deepCopy(lastConf.Groups)}
	newConf.Buckets[bid] = gid
	newConfigBytes, _ := json.Marshal(newConf)
	cfStm.dbEng.Put(CurVersionKey, strconv.Itoa(nextConfVersion))
	cfStm.dbEng.Put(CfPrefix+strconv.Itoa(nextConfVersion), string(newConfigBytes))
	cfStm.curConfVersion = nextConfVersion
	return nil
}

func (cfStm *PersistentConfigStm) Query(version int) (Config, error) {
	// if version is invalid, return latest config
	if version < 0 || version >= cfStm.curConfVersion {
		lastConf := &Config{}
		logger.ELogger().Sugar().Debugf("query cur version -> " + strconv.Itoa(cfStm.curConfVersion))
		confBytes, err := cfStm.dbEng.Get(CfPrefix + strconv.Itoa(cfStm.curConfVersion))
		if err != nil {
			return DefaultConfig(), err
		}
		json.Unmarshal([]byte(confBytes), lastConf)
		return *lastConf, nil
	}
	// return specified version config
	confBytes, err := cfStm.dbEng.Get(CfPrefix + strconv.Itoa(version))
	if err != nil {
		return DefaultConfig(), err
	}
	specConf := &Config{}
	json.Unmarshal([]byte(confBytes), specConf)
	return *specConf, nil
}
