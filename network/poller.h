#pragma once

#include <vector>

enum EventType
{
    EventTypeRead = 0x1,
    EventTypeWrite = 0x1 << 1,
    EventTypeError = 0x1 << 2,
};

struct FiredEvent
{
    int events;
    void* userdata;

    FiredEvent() : events(0), userdata(0)
    {
    }
};

class Poller
{
public:
    Poller() : multiplexer_(-1)
    {
    }

    virtual ~Poller()
    {
    }
    
    virtual bool AddSocket(int sock, int events, void* userPtr) = 0;
    virtual bool ModSocket(int sock, int events, void* userPtr) = 0;
    virtual bool DelSocket(int sock, int events) = 0;

    virtual int Poll(std::vector<FiredEvent>& events, std::size_t maxEv, int timeoutMs) = 0;

protected:
    int multiplexer_;
};
