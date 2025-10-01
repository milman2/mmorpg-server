#include "common/logger.hpp"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

namespace mmorpg::common {

std::shared_ptr<spdlog::logger> Logger::logger_ = nullptr;
bool Logger::initialized_ = false;

void Logger::initialize(const std::string& log_file) {
    if (initialized_) {
        return;
    }
    
    try {
        // 콘솔 출력용 싱크
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::info);
        console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%t] %v");
        
        // 파일 출력용 싱크 (회전 로그)
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            log_file, 1024 * 1024 * 10, 5); // 10MB, 5개 파일
        file_sink->set_level(spdlog::level::debug);
        file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%t] %v");
        
        // 로거 생성
        logger_ = std::make_shared<spdlog::logger>("mmorpg", 
            spdlog::sinks_init_list{console_sink, file_sink});
        
        logger_->set_level(spdlog::level::debug);
        logger_->flush_on(spdlog::level::warn);
        
        // 전역 로거로 설정
        spdlog::set_default_logger(logger_);
        
        initialized_ = true;
        
        LOG_INFO("Logger initialized successfully");
        
    } catch (const spdlog::spdlog_ex& ex) {
        std::cerr << "Log initialization failed: " << ex.what() << std::endl;
    }
}

void Logger::shutdown() {
    if (initialized_) {
        LOG_INFO("Logger shutting down");
        spdlog::shutdown();
        initialized_ = false;
    }
}

std::shared_ptr<spdlog::logger> Logger::get_logger() {
    if (!initialized_) {
        initialize();
    }
    return logger_;
}

} // namespace mmorpg::common


