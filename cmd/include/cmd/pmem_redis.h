#include <network/raft_stack.h>
#include <network/server.h>

#define VERSION "1.0.0"

class PMemRedis : public Server {
 public:
  PMemRedis();
  ~PMemRedis();

  static network::RaftStack* MakeStackInstance(
      std::shared_ptr<network::RaftConfig> raftConf) {
    if (instance_ == nullptr) {
      instance_ = new network::RaftStack(raftConf);
      return instance_;
    }
    return instance_;
  }

  static network::RaftStack* GetStackInstance() { return instance_; }

  static network::RaftStack* instance_;

  // bool ParseArgs(int ac, char* av[]);
  void InitSpdLogger();

 private:
  std::shared_ptr<StreamSocket> _OnNewConnection(int fd, int tag) override;

  bool _Init() override;
  bool _RunLogic() override;
  bool _Recycle() override;

  unsigned short port_;
};
