#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <set>
#include <memory>
#include <time.h>

enum LogLevel
{
    logINFO     = 0x01 << 0,
    logDEBUG    = 0x01 << 1,
    logWARN     = 0x01 << 2,
    logERROR    = 0x01 << 3,
    logUSR      = 0x01 << 4,
    logALL      = 0xFFFFFFFF,
};

enum LogDest
{
    logConsole  = 0x01 << 0,
    logFILE     = 0x01 << 1,
};

unsigned int ConvertLogLevel(const std::string& level);

class Logger
{
public:

    Logger(const Logger& ) = delete;
    void operator= (const Logger& ) = delete;

    bool Init(unsigned int level = logDEBUG,
              unsigned int dest = logConsole,
              const char* pDir  = 0);
    
    void Flush(LogLevel  level);
    bool IsLevelForbid(unsigned int level)  {  return  !(level & level_); };

    Logger&  operator<<(const char* msg);
    Logger&  operator<<(const unsigned char* msg);
    Logger&  operator<<(const std::string& msg);
    Logger&  operator<<(void* );
    Logger&  operator<<(unsigned char a);
    Logger&  operator<<(char a);
    Logger&  operator<<(unsigned short a);
    Logger&  operator<<(short a);
    Logger&  operator<<(unsigned int a);
    Logger&  operator<<(int a);
    Logger&  operator<<(unsigned long a);
    Logger&  operator<<(long a);
    Logger&  operator<<(unsigned long long a);
    Logger&  operator<<(long long a);
    Logger&  operator<<(double a);

    Logger& SetCurLevel(unsigned int level)
    {
        curLevel_ = level;
        return *this;
    }

    bool   Update();

    const std::string& _MakeFileName();
    Logger();
   ~Logger();


    static const size_t MAXLINE_LOG = 2048; // TODO
    char            tmpBuffer_[MAXLINE_LOG];
    std::size_t     pos_;

    time_t t_;
    
    unsigned int    level_;
    std::string     directory_;
    unsigned int    dest_;
    std::string     fileName_;

    unsigned int    curLevel_;
    
    // for optimization
    uint64_t        lastLogSecond_;
    uint64_t        lastLogMSecond_;
    
    std::size_t     _Log(const char* data, std::size_t len);

    bool    _CheckChangeFile();
    bool    _OpenLogFile(const char* name);
    void    _CloseLogFile();
    void    _WriteLog(int level, std::size_t nLen, const char* data);
    void    _Color(unsigned int color);
    void    _Reset();
};




#endif