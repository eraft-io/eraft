#include "config.h"
#include "toml.h"
#include <fstream>

Config g_config;

bool LoadServerConfig(const char *cfgFile, Config &cfg)
{
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
