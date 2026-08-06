#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <mutex>

enum LogLevel {
    LOG_DEBUG = 0,
    LOG_INFO  = 1,
    LOG_WARN  = 2,
    LOG_ERROR = 3
};

class Logger {
public:
    static Logger& Instance();
    void SetLogLevel(LogLevel level);
    void SetOutputFile(const std::string& filename);
    void Log(LogLevel level, const std::string& message);
    void Flush();

private:
    Logger();
    ~Logger();
    std::ofstream m_file;
    LogLevel m_level;
    std::mutex m_mutex;
    std::string m_filename;
    bool m_initialized;
};

#define LOG_DEBUG(msg) Logger::Instance().Log(LOG_DEBUG, msg)
#define LOG_INFO(msg)  Logger::Instance().Log(LOG_INFO, msg)
#define LOG_WARN(msg)  Logger::Instance().Log(LOG_WARN, msg)
#define LOG_ERROR(msg) Logger::Instance().Log(LOG_ERROR, msg)

#define LOG_ENTRY() LOG_DEBUG(std::string(__FUNCTION__) + " 进入")
#define LOG_EXIT()  LOG_DEBUG(std::string(__FUNCTION__) + " 退出")

#endif