# Copyright [2022] [WellWood] [wellwood-x@googlegroups.com]

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

# 	http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

default: meta_server block_server wellwood-ctl

meta_server:
	go build -o output/meta_server cmd/meta-server/main.go

block_server:
	go build -o output/block_server cmd/block-server/main.go

wellwood-ctl:
	go build -o output/wellwood-ctl cmd/sdk-ctl/main.go

clean:
	rm -rf output/*
