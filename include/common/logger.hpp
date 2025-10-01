#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <memory>
#include <string>

namespace mmorpg::common {

/**
 * @brief 로거 초기화 및 관리
 */
class Logger {
public:
    static void initialize(const std::string& log_file = "mmorpg_server.log");
    static void shutdown();
    static std::shared_ptr<spdlog::logger> get_logger();
    
private:
    static std::shared_ptr<spdlog::logger> logger_;
    static bool initialized_;
};

// 편의 매크로들
#define LOG_TRACE(...)    spdlog::trace(__VA_ARGS__)
#define LOG_DEBUG(...)    spdlog::debug(__VA_ARGS__)
#define LOG_INFO(...)     spdlog::info(__VA_ARGS__)
#define LOG_WARNING(...)  spdlog::warn(__VA_ARGS__)
#define LOG_ERROR(...)    spdlog::error(__VA_ARGS__)
#define LOG_CRITICAL(...) spdlog::critical(__VA_ARGS__)

} // namespace mmorpg::common


