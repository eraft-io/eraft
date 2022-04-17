#pragma once

#include <vector>
#include <map>
#include <mutex>
#include <memory>
#include <atomic>

class StreamSocket;

namespace Internal
{

class TaskManager
{
    typedef std::shared_ptr<StreamSocket> PTCPSOCKET;
    typedef std::vector<PTCPSOCKET> NEWTASKS_T;

public:
    TaskManager() : newCnt_(0) {}
    ~TaskManager();

    bool AddTask(PTCPSOCKET );

    bool Empty() const { return tcpSockets_.empty(); }
    void Clear() { tcpSockets_.clear(); }
    PTCPSOCKET FindTCP(unsigned int id) const;

    size_t TCPSize() const { return tcpSockets_.size(); }

    bool DoMsgParse();

private:
    bool _AddTask(PTCPSOCKET task);
    void _RemoveTask(std::map<int, PTCPSOCKET>::iterator& );
    std::map<int, PTCPSOCKET> tcpSockets_;

    std::mutex lock_;
    NEWTASKS_T newTasks_;
    std::atomic<int> newCnt_;
};

}
