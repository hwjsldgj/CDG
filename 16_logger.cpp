#include "16_logger.h"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <iostream>
#include <direct.h>
#include <sys/stat.h>

Logger& Logger::Instance() {
    static Logger instance;
    return instance;
}

Logger::Logger()
    : m_level(LOG_INFO)
    , m_initialized(false) {}

Logger::~Logger() {
    if (m_file.is_open()) {
        m_file << "[日志] 日志关闭。" << std::endl;
        m_file.close();
    }
}

void Logger::SetLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_level = level;
}

void Logger::SetOutputFile(const std::string& filename) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_initialized) {
        m_file.close();
        m_initialized = false;
    }

    size_t pos = filename.find_last_of("/\\");
    if (pos != std::string::npos) {
        std::string dir = filename.substr(0, pos);
        struct stat st;
        if (stat(dir.c_str(), &st) != 0) {
            if (_mkdir(dir.c_str()) != 0) {
                std::cerr << "创建日志目录失败：" << dir << std::endl;
                m_initialized = false;
                return;
            }
        }
    }

    m_filename = filename;
    m_file.open(m_filename, std::ios::out | std::ios::app);
    if (!m_file.is_open()) {
        std::cerr << "无法打开日志文件：" << m_filename << "，将使用控制台输出" << std::endl;
        m_initialized = false;
        return;
    }
    m_initialized = true;
    m_file << "[日志] 日志记录开始。" << std::endl;
}

void Logger::Log(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> lock(m_mutex);
    // 如果日志文件未初始化，输出到 stderr（始终可见）
    if (!m_initialized) {
        std::cerr << "[控制台日志] " << message << std::endl;
        return;
    }
    if (level < m_level) return;

    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    struct tm tm_buf;
    localtime_s(&tm_buf, &time);
    std::ostringstream oss;
    oss << "["
        << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S")
        << "." << std::setw(3) << std::setfill('0') << ms.count()
        << "][";
    switch (level) {
        case LOG_DEBUG: oss << "调试"; break;
        case LOG_INFO:  oss << "信息"; break;
        case LOG_WARN:  oss << "警告"; break;
        case LOG_ERROR: oss << "错误"; break;
        default:        oss << "未知"; break;
    }
    oss << "] " << message << std::endl;
    m_file << oss.str();
    m_file.flush();
}

void Logger::Flush() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_file.is_open()) {
        m_file.flush();
    }
}