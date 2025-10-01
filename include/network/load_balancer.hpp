#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <mutex>
#include <chrono>
#include <memory>

namespace mmorpg::network {

/**
 * @brief 서버 노드 정보
 */
struct ServerNode {
    std::string id;
    std::string host;
    uint16_t port;
    std::atomic<uint32_t> current_connections{0};
    std::atomic<uint32_t> max_connections{1000};
    std::atomic<double> cpu_usage{0.0};
    std::atomic<double> memory_usage{0.0};
    std::atomic<bool> is_healthy{true};
    std::chrono::time_point<std::chrono::steady_clock> last_health_check;
    
    // 부하 점수 계산
    double get_load_score() const {
        double connection_ratio = static_cast<double>(current_connections.load()) / max_connections.load();
        double cpu_weight = cpu_usage.load() * 0.4;
        double memory_weight = memory_usage.load() * 0.3;
        double connection_weight = connection_ratio * 0.3;
        
        return cpu_weight + memory_weight + connection_weight;
    }
    
    // 서버 용량 확인
    bool can_accept_connection() const {
        return is_healthy.load() && 
               current_connections.load() < max_connections.load() &&
               get_load_score() < 0.8;
    }
};

/**
 * @brief 로드 밸런싱 전략
 */
enum class LoadBalancingStrategy {
    ROUND_ROBIN,      // 라운드 로빈
    LEAST_CONNECTIONS, // 최소 연결 수
    LEAST_LOAD,       // 최소 부하
    WEIGHTED_ROUND_ROBIN, // 가중치 라운드 로빈
    IP_HASH          // IP 해시
};

/**
 * @brief 로드 밸런서
 */
class LoadBalancer {
public:
    explicit LoadBalancer(LoadBalancingStrategy strategy = LoadBalancingStrategy::LEAST_LOAD);
    ~LoadBalancer() = default;
    
    /**
     * @brief 로드 밸런서 시작
     */
    void start();
    
    /**
     * @brief 로드 밸런서 중지
     */
    void stop();
    
    /**
     * @brief 서버 노드 추가
     */
    void add_server(const std::string& id, const std::string& host, uint16_t port, uint32_t max_connections = 1000);
    
    /**
     * @brief 서버 노드 제거
     */
    void remove_server(const std::string& id);
    
    /**
     * @brief 최적 서버 선택
     */
    std::string select_server(const std::string& client_ip = "");
    
    /**
     * @brief 연결 할당
     */
    bool assign_connection(const std::string& server_id, const std::string& connection_id);
    
    /**
     * @brief 연결 해제
     */
    void release_connection(const std::string& server_id, const std::string& connection_id);
    
    /**
     * @brief 서버 상태 업데이트
     */
    void update_server_status(const std::string& server_id, double cpu_usage, double memory_usage, bool is_healthy);
    
    /**
     * @brief 서버 정보 조회
     */
    const ServerNode* get_server(const std::string& server_id) const;
    
    /**
     * @brief 모든 서버 정보 조회
     */
    std::vector<ServerNode> get_all_servers() const;
    
    /**
     * @brief 로드 밸런싱 전략 변경
     */
    void set_strategy(LoadBalancingStrategy strategy);
    
    /**
     * @brief 헬스 체크 시작
     */
    void start_health_check(std::chrono::seconds interval = std::chrono::seconds(30));

private:
    void perform_health_check();
    std::string select_round_robin();
    std::string select_least_connections();
    std::string select_least_load();
    std::string select_weighted_round_robin();
    std::string select_ip_hash(const std::string& client_ip);
    
    LoadBalancingStrategy strategy_;
    std::vector<std::unique_ptr<ServerNode>> servers_;
    mutable std::mutex servers_mutex_;
    
    std::atomic<uint32_t> round_robin_index_{0};
    std::atomic<bool> running_{false};
    
    std::thread health_check_thread_;
    std::chrono::seconds health_check_interval_{30};
    
    // 연결 추적
    std::unordered_map<std::string, std::string> connection_to_server_;
    mutable std::mutex connection_mutex_;
};

} // namespace mmorpg::network


