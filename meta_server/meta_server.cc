#include <kv/config.h>
#include <kv/raft_server.h>
#include <kv/raft_store.h>
#include <kv/scheduler_server.h>
#include <kv/server.h>
#include <spdlog/common.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>

#include <memory>

int main(int argc, char* argv[]) {
  // init logger
  auto console = spdlog::stdout_logger_mt("console");
  spdlog::set_default_logger(console);
  spdlog::set_level(spdlog::level::debug);
  spdlog::set_pattern("[source %s] [function %!] [line %#] %v");

  std::shared_ptr<kvserver::Config> conf = std::make_shared<kvserver::Config>(
      std::string(argv[1]), std::string(argv[2]),
      std::stoi(std::string(argv[3])));

  // start raft store
  std::unique_ptr<kvserver::RaftStorage> storage(
      new kvserver::RaftStorage(conf));
  storage->Start();

  kvserver::ScheServer schSvr(conf->storeAddr_, storage.get());
  schSvr.RunLogic();

  SPDLOG_INFO("server start ok");
}
