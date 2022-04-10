#include "server.h"

#define VERSION "1.0.0"

class PMemRedis : public Server {
public:
	PMemRedis();
	~PMemRedis();

	// bool ParseArgs(int ac, char* av[]);

private:
	std::shared_ptr<StreamSocket> _OnNewConnection(int fd, int tag) override;

	bool _Init() override;
	bool _RunLogic() override;
	bool _Recycle() override;

	unsigned short port_;
};
