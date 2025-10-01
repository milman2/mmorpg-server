#include "common/logger.hpp"
#include "agents/connection_manager/connection_manager.hpp"
#include <iostream>
#include <signal.h>
#include <memory>

namespace mmorpg {

// 전역 변수로 에이전트 관리
std::unique_ptr<mmorpg::agents::connection_manager::ConnectionManagerAgent> connection_manager;

// 시그널 핸들러
void signal_handler(int signal) {
    std::cout << "\nReceived signal " << signal << ". Shutting down gracefully..." << std::endl;
    
    if (connection_manager) {
        connection_manager->stop();
    }
    
    mmorpg::common::Logger::shutdown();
    exit(0);
}

} // namespace mmorpg

int main(int argc, char* argv[]) {
    try {
        // 로거 초기화
        mmorpg::common::Logger::initialize("mmorpg_server.log");
        
        LOG_INFO("MMORPG Server starting...");
        LOG_INFO("Version: 1.0.0");
        LOG_INFO("Build: {}", __DATE__ " " __TIME__);
        
        // 시그널 핸들러 등록
        signal(SIGINT, mmorpg::signal_handler);
        signal(SIGTERM, mmorpg::signal_handler);
        
        // Connection Manager Agent 생성 및 시작
        connection_manager = std::make_unique<mmorpg::agents::connection_manager::ConnectionManagerAgent>(5000);
        
        LOG_INFO("Starting Connection Manager Agent...");
        connection_manager->start();
        
        LOG_INFO("MMORPG Server started successfully!");
        LOG_INFO("WebSocket server listening on port 8080");
        LOG_INFO("Maximum connections: 5000");
        
        // 메인 루프
        while (connection_manager->is_running()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            // 주기적으로 상태 출력
            static auto last_status_time = std::chrono::steady_clock::now();
            auto now = std::chrono::steady_clock::now();
            
            if (now - last_status_time > std::chrono::seconds(30)) {
                auto stats = connection_manager->get_connection_stats();
                LOG_INFO("Server Status - Connections: {:.0f}/{:.0f} ({:.1f}%)", 
                        stats["total_connections"], 
                        stats["max_connections"],
                        stats["connection_utilization"] * 100.0);
                
                last_status_time = now;
            }
        }
        
    } catch (const std::exception& e) {
        LOG_CRITICAL("Fatal error: {}", e.what());
        return 1;
    } catch (...) {
        LOG_CRITICAL("Unknown fatal error occurred");
        return 1;
    }
    
    LOG_INFO("MMORPG Server shutdown complete");
    return 0;
}


