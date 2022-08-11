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

package consts

const SLOT_NUM = 10

const MB = 1024 * 1024

const KB = 1024

const GB = 1024 * 1024 * 1024

const FILE_BLOCK_SIZE = 1024 * 1024 * 1

var BUCKET_META_PREFIX = []byte{0x01, 0x09, 0x09, 0x08}

var OBJECT_META_PREFIX = []byte{0x01, 0x09, 0x09, 0x05}

var TOPO_CONF_PREFIX = []byte{0x01, 0x09, 0x09, 0x07}

var CUR_TOPO_CONF_VERSION_KEY = []byte{0x01, 0x09, 0x09, 0x06}

var RAFTLOG_PREFIX = []byte{0x11, 0x11, 0x19, 0x96}

var FIRST_IDX_KEY = []byte{0x88, 0x88}

var LAST_IDX_KEY = []byte{0x99, 0x99}

var RAFT_STATE_KEY = []byte{0x19, 0x49}

const INIT_LOG_INDEX = 0

var SNAPSHOT_STATE_KEY = []byte{0x19, 0x97}

const MAX_GRPC_SEND_MSG_SIZE = 1024 * 1024 * 200

const MAX_GRPC_RECV_MSG_SIZE = 1024 * 1024 * 200

const DEFAULT_BUCKET_ID = "3f8602ef-9488-419f-a485-4df0cbd73c3"
