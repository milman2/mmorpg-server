#pragma once

#include "common/base_agent.hpp"
#include "network/websocket_handler.hpp"
#include "network/load_balancer.hpp"
#include <boost/asio.hpp>
#include <memory>
#include <unordered_map>
#include <atomic>
#include <mutex>

namespace mmorpg::agents::connection_manager {

/**
 * @brief 연결 정보를 담는 구조체
 */
struct ConnectionInfo {
    std::string connection_id;
    std::string user_id;
    std::string ip_address;
    std::chrono::time_point<std::chrono::high_resolution_clock> connected_at;
    std::chrono::time_point<std::chrono::high_resolution_clock> last_activity;
    bool is_authenticated = false;
    std::atomic<uint64_t> bytes_sent{0};
    std::atomic<uint64_t> bytes_received{0};
};

/**
 * @brief Connection Manager Agent
 * 
 * 클라이언트 연결을 관리하고 로드 밸런싱을 수행합니다.
 * 5,000명의 동시 접속자를 지원합니다.
 */
class ConnectionManagerAgent : public mmorpg::common::BaseAgent {
public:
    explicit ConnectionManagerAgent(uint32_t max_connections = 5000);
    ~ConnectionManagerAgent() override = default;

    void start() override;
    void stop() override;

    /**
     * @brief 새로운 연결 처리
     * @param connection_id 연결 ID
     * @param ip_address 클라이언트 IP 주소
     * @return 연결 허용 여부
     */
    bool handle_new_connection(const std::string& connection_id, 
                              const std::string& ip_address);

    /**
     * @brief 연결 해제 처리
     * @param connection_id 연결 ID
     */
    void handle_disconnection(const std::string& connection_id);

    /**
     * @brief 연결 인증 처리
     * @param connection_id 연결 ID
     * @param user_id 사용자 ID
     */
    void authenticate_connection(const std::string& connection_id, 
                                const std::string& user_id);

    /**
     * @brief 활동 시간 업데이트
     * @param connection_id 연결 ID
     */
    void update_activity(const std::string& connection_id);

    /**
     * @brief 연결 통계 반환
     */
    std::unordered_map<std::string, double> get_connection_stats() const;

    /**
     * @brief 특정 연결 정보 조회
     */
    std::optional<ConnectionInfo> get_connection_info(const std::string& connection_id) const;

    /**
     * @brief 비활성 연결 정리
     */
    void cleanup_inactive_connections(std::chrono::seconds timeout = std::chrono::seconds(300));

private:
    uint32_t max_connections_;
    std::atomic<uint32_t> current_connections_{0};
    
    mutable std::mutex connections_mutex_;
    std::unordered_map<std::string, std::unique_ptr<ConnectionInfo>> connections_;
    
    std::unique_ptr<network::WebSocketHandler> websocket_handler_;
    std::unique_ptr<network::LoadBalancer> load_balancer_;
    
    boost::asio::io_context io_context_;
    std::unique_ptr<boost::asio::io_context::work> work_;
    std::vector<std::thread> worker_threads_;
    
    void start_worker_threads();
    void stop_worker_threads();
};

} // namespace mmorpg::agents::connection_manager
