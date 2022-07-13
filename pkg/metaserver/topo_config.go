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

package metaserver

import (
	"bytes"
	"encoding/json"
	"strconv"

	"github.com/eraft-io/eraft/pkg/consts"
	"github.com/eraft-io/eraft/pkg/engine"
	"github.com/eraft-io/eraft/pkg/log"
)

type TopoConfig struct {
	Version      int
	Slots        [consts.SLOT_NUM]int
	ServerGroups map[int][]string
}

func DefaultTopoConfig() TopoConfig {
	return TopoConfig{
		ServerGroups: make(map[int][]string),
	}
}

func deepCopy(groups map[int][]string) map[int][]string {
	newG := make(map[int][]string)
	for gid, severs := range groups {
		newSvrs := make([]string, len(severs))
		copy(newSvrs, severs)
		newG[gid] = newSvrs
	}
	return newG
}

type TopoConfigSTM interface {
	Join(serverGroups map[int][]string) error
	Leave(groupIds []int) error
	Query(versionId int) (TopoConfig, error)
}

type PersisTopoConfigSTM struct {
	metaEng          engine.KvStore
	curConfigVersion int
}

func NewTopoConfigSTM(metaEng engine.KvStore) *PersisTopoConfigSTM {
	var buffer bytes.Buffer
	buffer.Write(consts.TOPO_CONF_PREFIX)
	buffer.Write([]byte(strconv.Itoa(0)))
	persisTopoConfigSTM := &PersisTopoConfigSTM{
		metaEng:          metaEng,
		curConfigVersion: 0,
	}
	_, err := metaEng.Get(buffer.Bytes())
	if err != nil {
		// do not have default topo config
		defaultTopoConfig := DefaultTopoConfig()
		defaultTopoConfigBytes, err := json.Marshal(defaultTopoConfig)
		if err != nil {
			log.MainLogger.Error().Msgf("marshal default topo conf err %s", err.Error())
			return nil
		}
		if err := persisTopoConfigSTM.metaEng.Put(buffer.Bytes(), defaultTopoConfigBytes); err != nil {
			log.MainLogger.Error().Msgf("persis default topo conf err %s", err.Error())
			return nil
		}
		if err := persisTopoConfigSTM.metaEng.Put(consts.CUR_TOPO_CONF_VERSION_KEY, []byte(strconv.Itoa(persisTopoConfigSTM.curConfigVersion))); err != nil {
			log.MainLogger.Error().Msgf("persis cur conf version err %s", err.Error())
			return nil
		}
		return persisTopoConfigSTM
	}
	versionSeq, err := metaEng.Get(consts.CUR_TOPO_CONF_VERSION_KEY)
	if err != nil {
		log.MainLogger.Error().Msgf("get cur conf version err %s", err.Error())
		return nil
	}
	versionInt, _ := strconv.Atoi(string(versionSeq))
	persisTopoConfigSTM.curConfigVersion = versionInt
	return persisTopoConfigSTM
}

func (topoConfStm *PersisTopoConfigSTM) Join(serverGroups map[int][]string) error {
	var buffer bytes.Buffer
	buffer.Write(consts.TOPO_CONF_PREFIX)
	buffer.Write([]byte(strconv.Itoa(topoConfStm.curConfigVersion)))
	topoConfByteSeq, err := topoConfStm.metaEng.Get(buffer.Bytes())
	if err != nil {
		return err
	}
	lastTopoConf := &TopoConfig{}
	json.Unmarshal(topoConfByteSeq, lastTopoConf)
	newTopoConfig := TopoConfig{
		topoConfStm.curConfigVersion + 1,
		lastTopoConf.Slots,
		deepCopy(lastTopoConf.ServerGroups),
	}
	for gid, servers := range serverGroups {
		if _, ok := newTopoConfig.ServerGroups[gid]; !ok {
			newSvrs := make([]string, len(servers))
			copy(newSvrs, servers)
			newTopoConfig.ServerGroups[gid] = newSvrs
		}
	}
	var gidList []int
	for gid := range newTopoConfig.ServerGroups {
		gidList = append(gidList, gid)
	}
	// set slot
	step := 0
	for i := 0; i < consts.SLOT_NUM; i++ {
		newTopoConfig.Slots[i] = gidList[step]
		step += 1
		if step >= len(gidList) {
			step = 0
		}
	}
	newTopoConfigByteSeq, _ := json.Marshal(newTopoConfig)
	if err := topoConfStm.metaEng.Put(consts.CUR_TOPO_CONF_VERSION_KEY, []byte(strconv.Itoa(topoConfStm.curConfigVersion+1))); err != nil {
		return err
	}
	var newTopoConfKeyBuffer bytes.Buffer
	newTopoConfKeyBuffer.Write(consts.TOPO_CONF_PREFIX)
	newTopoConfKeyBuffer.Write([]byte(strconv.Itoa(topoConfStm.curConfigVersion + 1)))
	if err := topoConfStm.metaEng.Put(newTopoConfKeyBuffer.Bytes(), newTopoConfigByteSeq); err != nil {
		return err
	}
	topoConfStm.curConfigVersion += 1
	return nil
}

func (topoConfStm *PersisTopoConfigSTM) Query(version int) (TopoConfig, error) {
	if version < 0 || version >= topoConfStm.curConfigVersion {
		lastTopoConf := &TopoConfig{}
		var buffer bytes.Buffer
		buffer.Write(consts.TOPO_CONF_PREFIX)
		buffer.Write([]byte(strconv.Itoa(topoConfStm.curConfigVersion)))
		topoConfBytes, err := topoConfStm.metaEng.Get(buffer.Bytes())
		if err != nil {
			return DefaultTopoConfig(), err
		}
		json.Unmarshal(topoConfBytes, lastTopoConf)
		return *lastTopoConf, nil
	}
	var topoConfKeyBuffer bytes.Buffer
	topoConfKeyBuffer.Write(consts.TOPO_CONF_PREFIX)
	topoConfKeyBuffer.Write([]byte(strconv.Itoa(version)))
	topoConfBytes, err := topoConfStm.metaEng.Get(topoConfKeyBuffer.Bytes())
	if err != nil {
		return DefaultTopoConfig(), err
	}
	queryTopoConf := &TopoConfig{}
	json.Unmarshal(topoConfBytes, queryTopoConf)
	return *queryTopoConf, nil
}

func (topoConfStm *PersisTopoConfigSTM) Leave(groupIds []int) error {
	var buffer bytes.Buffer
	buffer.Write(consts.TOPO_CONF_PREFIX)
	buffer.Write([]byte(strconv.Itoa(topoConfStm.curConfigVersion)))
	topoConfBytes, err := topoConfStm.metaEng.Get(buffer.Bytes())
	if err != nil {
		return err
	}
	lastTopoConf := &TopoConfig{}
	json.Unmarshal(topoConfBytes, lastTopoConf)
	newTopoConf := TopoConfig{topoConfStm.curConfigVersion + 1, lastTopoConf.Slots, deepCopy(lastTopoConf.ServerGroups)}
	for _, gid := range groupIds {
		delete(newTopoConf.ServerGroups, gid)
	}
	var gidList []int
	for gid := range newTopoConf.ServerGroups {
		gidList = append(gidList, gid)
	}
	// set slot
	step := 0
	for i := 0; i < consts.SLOT_NUM; i++ {
		newTopoConf.Slots[i] = gidList[step]
		step += 1
		if step >= len(gidList) {
			step = 0
		}
	}
	newTopoConfByteSeq, _ := json.Marshal(newTopoConf)
	if err := topoConfStm.metaEng.Put(consts.CUR_TOPO_CONF_VERSION_KEY, []byte(strconv.Itoa(topoConfStm.curConfigVersion+1))); err != nil {
		return err
	}
	var newTopoConfKeyBuffer bytes.Buffer
	newTopoConfKeyBuffer.Write(consts.TOPO_CONF_PREFIX)
	newTopoConfKeyBuffer.Write([]byte(strconv.Itoa(topoConfStm.curConfigVersion + 1)))
	if err := topoConfStm.metaEng.Put(newTopoConfKeyBuffer.Bytes(), newTopoConfByteSeq); err != nil {
		return err
	}
	topoConfStm.curConfigVersion += 1
	return nil
}
