#define SPDLOG_HEADER_ONLY
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/pattern_formatter.h>
#include "Logger.h"
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <ctime>

namespace Logger {

// ──── Logger ────

Logger &Logger::instance()
{
    static Logger inst;
    return inst;
}

Logger::~Logger()
{
    // 不在此处调用 shutdown()：程序退出时静态析构顺序不确定，
    // spdlog 内部 registry 可能已先被销毁，访问会导致崩溃。
    // 进程退出时 OS 会回收所有资源，无需手动清理。
}

void Logger::initialize(Output output, Level level, const std::string &fileDir)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (initialized_) return;

    level_ = level;

    auto spdLogger = std::make_shared<spdlog::logger>("vcc");

    // 控制台输出（带颜色）
    if (output == Output::Console || output == Output::Both) {
        auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%s:%#] [tid:%t] %v");
        // 使用自定义 formatter 仅显示文件名而非完整路径
        sink->set_formatter(std::make_unique<spdlog::pattern_formatter>(
            "[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%s:%#] [tid:%t] %v",
            spdlog::pattern_time_type::local, ""));
        spdLogger->sinks().push_back(sink);
    }

    // 文件输出
    if ((output == Output::File || output == Output::Both) && !fileDir.empty()) {
        std::filesystem::create_directories(fileDir);
        std::string path = fileDir;
        if (path.back() != '/' && path.back() != '\\') path += "/";
        path += logFileName();

        auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path, true);
        sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%s:%#] [tid:%t] %v");
        spdLogger->sinks().push_back(sink);
    }

    // 等级转换
    static const spdlog::level::level_enum kMap[] = {
        spdlog::level::debug,   // Debug
        spdlog::level::info,    // Info
        spdlog::level::warn,    // Warn
        spdlog::level::err,     // Error
        spdlog::level::off,     // Off
    };
    spdLogger->set_level(kMap[static_cast<int>(level)]);
    spdLogger->flush_on(spdlog::level::trace);

    spdlog::register_logger(spdLogger);
    spdLogger_ = spdLogger;
    initialized_ = true;
}

void Logger::shutdown()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!initialized_) return;

    if (spdLogger_) {
        auto spd = std::static_pointer_cast<spdlog::logger>(spdLogger_);
        spd->flush();
        spdlog::drop("vcc");
        spdLogger_.reset();
    }
    initialized_ = false;
}

void Logger::log(Level level, const char *file, int line, const std::string &msg)
{
    if (!initialized_) return;
    std::lock_guard<std::mutex> lock(mutex_);

    auto spd = std::static_pointer_cast<spdlog::logger>(spdLogger_);
    if (!spd) return;

    // 仅保留文件名
    const char *fname = strrchr(file, '/')  ? strrchr(file, '/') + 1
                      : strrchr(file, '\\') ? strrchr(file, '\\') + 1 : file;

    static const spdlog::level::level_enum kMap[] = {
        spdlog::level::debug, spdlog::level::info,
        spdlog::level::warn,  spdlog::level::err, spdlog::level::off
    };
    spd->log(spdlog::source_loc{file, line, fname},
             kMap[static_cast<int>(level)], msg);
}

void Logger::logStream(Level level, const char *file, int line, const std::string &msg)
{
    log(level, file, line, msg);
}

std::string Logger::logFileName()
{
    auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm tm;
    localtime_s(&tm, &t);
    std::ostringstream ss;
    ss << "vcc_" << std::put_time(&tm, "%Y_%m_%d_%H_%M_%S") << ".log";
    return ss.str();
}

// ──── LogStreamHelper ────

LogStreamHelper::LogStreamHelper(Level level, const char *file, int line)
    : level_(level), file_(file), line_(line)
{
}

LogStreamHelper::~LogStreamHelper()
{
    Logger::instance().logStream(level_, file_, line_, stream_.str());
}

} // namespace Logger
