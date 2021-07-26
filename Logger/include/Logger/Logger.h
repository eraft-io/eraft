#ifndef _LOGGER_H_
#define _LOGGER_H_

#include <iostream>
#include <fstream>
#include <time.h>
#include <stdint.h>

class Logger
{
public:

    enum log_level 
    { 
        debug, 
        info, 
        warning, 
        error
    };

    enum log_target 
    { 
        file,
        terminal,
        file_and_terminal
    };

public:
    Logger();
    Logger(log_target target, log_level level, const std::string& path);
    ~Logger();
    
    void DEBUG_NEW(const std::string& in, const std::string& file, uint64_t line, const std::string& function);
    void DEBUG(const std::string& text);
    void INFO(const std::string& text);
    void WARNING(const std::string& text);
    void ERRORS(const std::string& text);

    static Logger* GetInstance()
    {
        if(instance_ == nullptr)
        {
            instance_ = new Logger(Logger::terminal, Logger::debug, "Log.log");
        }
        return instance_;
    }

private:
    static Logger* instance_;

    std::ofstream m_outfile;    // 将日志输出到文件的流对象
    log_target m_target;        // 日志输出目标
    std::string m_path;              // 日志文件路径
    log_level m_level;          // 日志等级
    void output(const std::string &text, log_level act_level);            // 输出行为
};

#endif//_LOGGER_H_
