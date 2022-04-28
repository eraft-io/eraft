#include <network/client.h>
#include <network/command.h>
#include <network/config.h>
#include <network/executor.h>
#include <network/raft_config.h>
#include <network/raft_stack.h>
#include <network/server.h>
#include <network/socket.h>
#include <spdlog/common.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>

#include <memory>

#define VERSION "1.0.0"

class PMemRedis : public Server {
 public:
  PMemRedis();
  ~PMemRedis();

  // bool ParseArgs(int ac, char* av[]);
  void InitSpdLogger();

  std::shared_ptr<network::RaftStack> GetRaftStack();

  static PMemRedis *GetInstance() {
    if (instance_ == nullptr) {
      instance_ = new PMemRedis();
      return instance_;
    }
    return instance_;
  }

 protected:
  static PMemRedis *instance_;

 private:
  std::shared_ptr<StreamSocket> _OnNewConnection(int fd, int tag) override;
  std::shared_ptr<network::RaftStack> raftStack_;

  bool _Init() override;
  bool _RunLogic() override;
  bool _Recycle() override;

  unsigned short port_;
};
