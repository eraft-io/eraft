#pragma once

#include <map>
#include <string>
#include <vector>

struct Config {
	// [server]
	std::string listenAddr;

	std::string logDir;

	// [db]
	std::string dbPath;
};

extern Config g_config;

extern bool LoadServerConfig(const char *cfgFile, Config &cfg);
