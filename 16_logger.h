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

// ---- 基础日志宏 ----
#define LOG_DEBUG(msg) Logger::Instance().Log(LOG_DEBUG, msg)
#define LOG_INFO(msg)  Logger::Instance().Log(LOG_INFO, msg)
#define LOG_WARN(msg)  Logger::Instance().Log(LOG_WARN, msg)
#define LOG_ERROR(msg) Logger::Instance().Log(LOG_ERROR, msg)

// ---- 函数进入/退出宏（带缩进） ----
#define LOG_ENTRY() LOG_DEBUG(std::string(">> ") + __FUNCTION__)
#define LOG_EXIT()  LOG_DEBUG(std::string("<< ") + __FUNCTION__)

// ---- 别名（兼容旧版） ----
#define LOG_FUNC_ENTER() LOG_ENTRY()
#define LOG_FUNC_EXIT()  LOG_EXIT()

// ---- 条件日志 ----
#define LOG_IF_INFO(cond, msg)   if (cond) LOG_INFO(msg)
#define LOG_IF_WARN(cond, msg)   if (cond) LOG_WARN(msg)
#define LOG_IF_ERROR(cond, msg)  if (cond) LOG_ERROR(msg)

// ---- 内存分配日志 ----
#define LOG_MEM_ALLOC(size) LOG_DEBUG(std::string("内存分配 ") + std::to_string(size) + " 字节")
#define LOG_MEM_FAIL()       LOG_ERROR("内存分配失败")

#endif