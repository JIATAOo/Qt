#pragma once

#include "LogLevel.h"
#include <string>
#include <memory>
#include <sstream>
#include <mutex>

namespace Logger {

class LogStreamHelper;

// ──── 单例 Logger ────

class Logger
{
public:
    static Logger &instance();

    void initialize(Output output, Level level, const std::string &fileDir = "");
    void shutdown();
    bool ready() const { return initialized_; }

    void log(Level level, const char *file, int line, const std::string &msg);
    void logStream(Level level, const char *file, int line, const std::string &msg);

private:
    Logger() = default;
    ~Logger();
    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;

    std::string logFileName();

    bool initialized_ = false;
    Level level_ = Level::Debug;
    std::mutex mutex_;
    std::shared_ptr<void> spdLogger_;  // 类型擦除，避免头文件暴露 spdlog
};

// ──── 流式辅助类 ────

class LogStreamHelper
{
public:
    LogStreamHelper(Level level, const char *file, int line);
    ~LogStreamHelper();

    template<typename T>
    LogStreamHelper &operator<<(const T &v) { stream_ << v; return *this; }

private:
    Level level_;
    const char *file_;
    int line_;
    std::ostringstream stream_;
};

} // namespace Logger

// ──── 字符串宏 ────

#define LOG_DEBUG(msg) \
    Logger::Logger::instance().log(Logger::Level::Debug, __FILE__, __LINE__, msg)

#define LOG_INFO(msg) \
    Logger::Logger::instance().log(Logger::Level::Info,  __FILE__, __LINE__, msg)

#define LOG_WARN(msg) \
    Logger::Logger::instance().log(Logger::Level::Warn,  __FILE__, __LINE__, msg)

#define LOG_ERROR(msg) \
    Logger::Logger::instance().log(Logger::Level::Error, __FILE__, __LINE__, msg)

// ──── 流式宏 ────

#define LOG_DEBUG_S  Logger::LogStreamHelper(Logger::Level::Debug, __FILE__, __LINE__)
#define LOG_INFO_S   Logger::LogStreamHelper(Logger::Level::Info,  __FILE__, __LINE__)
#define LOG_WARN_S   Logger::LogStreamHelper(Logger::Level::Warn,  __FILE__, __LINE__)
#define LOG_ERROR_S  Logger::LogStreamHelper(Logger::Level::Error, __FILE__, __LINE__)

// ──── 参考项目兼容别名 ────

#define LOG_DEBUG_STREAM  Logger::LogStreamHelper(Logger::Level::Debug, __FILE__, __LINE__)
#define LOG_INFO_STREAM   Logger::LogStreamHelper(Logger::Level::Info,  __FILE__, __LINE__)
#define LOG_WARN_STREAM   Logger::LogStreamHelper(Logger::Level::Warn,  __FILE__, __LINE__)
#define LOG_ERROR_STREAM  Logger::LogStreamHelper(Logger::Level::Error, __FILE__, __LINE__)
