package clientsdk

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

import (
	"testing"

	"github.com/eraft-io/eraft/pkg/clientsdk"
)

func TestUpload(t *testing.T) {
	wellWoodCli := clientsdk.NewClient("127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090", "", "")
	wellWoodCli.UploadFile("/Users/colin/Documents/test_flex.pdf", "3f8602ef-9488-419f-a485-4df0cbd73c3")
}
