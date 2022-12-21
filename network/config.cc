// Copyright 2022 The uhp-sql Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include "config.h"

#include <fstream>

#include "toml.h"

Config g_config;

bool LoadServerConfig(const char *cfgFile, Config &cfg) {
  std::ifstream ifs(cfgFile);
  toml::ParseResult pr = toml::parse(ifs);

  if (!pr.valid()) {
    return false;
  }

  const toml::Value &v = pr.value;

  const toml::Value *addr = v.find("server.addr");
  if (addr && addr->is<std::string>()) {
    cfg.listenAddr = addr->as<std::string>();
  } else {
    return false;
  }
  const toml::Value *logDir = v.find("server.logdir");
  if (logDir && logDir->is<std::string>()) {
    cfg.logDir = logDir->as<std::string>();
  } else {
    return false;
  }

  const toml::Value *path = v.find("db.path");
  if (path && path->is<std::string>()) {
    cfg.dbPath = path->as<std::string>();
  } else {
    return false;
  }

  return true;
}
