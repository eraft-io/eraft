#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <errno.h>
#include <sys/stat.h>
#include <Logger/Logger.h>

static const size_t DEFAULT_LOGFILESIZE = 32 * 1024 * 1024;
static const size_t PREFIX_LEVEL_LEN    = 6;
static const size_t PREFIX_TIME_LEN     = 24;

unsigned int ConvertLogLevel(const std::string& level)
{
    unsigned int l = logALL;
    
    if (level == "debug")
    {
        ;
    }
    else if (level == "verbose")
    {
        l &= ~logDEBUG;
    }
    else if (level == "notice")
    {
        l &= ~logDEBUG;
        l &= ~logINFO;
    }
    else if (level == "warning")
    {
        l &= ~logDEBUG;
        l &= ~logINFO;
        l &= ~logWARN; // redis warning is my error
    }
    else if (level == "none")
    {
        l = 0;
    }
    
    return l;
}

static bool MakeDir(const char* pDir)
{
    if (mkdir(pDir, 0755) != 0)
    {
        if (EEXIST != errno)
            return false;
    }
    
    return true;
}

__thread Logger*  g_log = nullptr;
__thread unsigned g_logLevel;
__thread unsigned g_logDest;

Logger::Logger() : level_(0),
                   dest_(0)
{
    lastLogMSecond_ = lastLogSecond_ = -1;
}

Logger::~Logger()
{

}

bool Logger::Init(unsigned int level, unsigned int dest, const char* pDir)
{
    level_ = level;
    dest_ = dest;
    directory_ = pDir ? pDir : ".";

    if (0 == level_) 
    {
        return true;
    }

    if (!(dest_ & logConsole)) 
    {
        std::cerr << "log has no output, but loglevel is " << level << std::endl;
        return false;
    }

    return true;
}

bool Logger::CheckChangeFile()
{
    return false;
}

const std::string& Logger::MakeFileName()
{
    char buf[50];
    time_t rawtime;
    struct tm * timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(buf, 50, "%F.%T", timeinfo);

    fileName_ = directory_ + "/" + std::string(buf);
    fileName_ += ".log";

    return fileName_;
}
    
bool Logger::OpenLogFile(const char* name)
{
    return false;
}

void Logger::CloseLogFile()
{

}

void Logger::Flush(LogLevel  level)
{

}

void Logger::Color(unsigned int color)
{

}

Logger&  Logger::operator<<(const char* msg)
{
    return *this;
}

Logger&  Logger::operator<<(const unsigned char* msg)
{
    return *this;
}

Logger&  Logger::operator<<(const std::string& msg)
{
    return *this;
}


Logger& Logger::operator<<(void* )
{
    return *this;

}

Logger&  Logger::operator<<(unsigned char a)
{
    return *this;

}

Logger&  Logger::operator<<(char a)
{
    return *this;

}

Logger&  Logger::operator<<(unsigned short a)
{
    return *this;

}


Logger&  Logger::operator<<(short a)
{
    return *this;

}

Logger&  Logger::operator<<(unsigned int a)
{
    return *this;

}

Logger&  Logger::operator<<(int a)
{
    return *this;
}

Logger&  Logger::operator<<(unsigned long a)
{
    return *this;
}

Logger&  Logger::operator<<(long a)
{
    return *this;

}

Logger&  Logger::operator<<(unsigned long long a)
{
    return *this;
}

Logger&  Logger::operator<<(long long a)
{
    return *this;
}

Logger&  Logger::operator<<(double a)
{
    return *this;
}

bool Logger::Update()
{
    return false;
}

void Logger::Reset()
{

}

size_t Logger::Log(const char* data, size_t dataLen)
{
    return 0;
}

void Logger::WriteLog(int level, size_t nLen, const char* data)
{

}
