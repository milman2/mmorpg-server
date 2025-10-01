#include "agents/connection_manager/connection_manager.hpp"
#include "common/logger.hpp"
#include <algorithm>
#include <thread>
#include <stdexcept>

namespace mmorpg::agents::connection_manager {

ConnectionManagerAgent::ConnectionManagerAgent(uint32_t max_connections)
    : BaseAgent("ConnectionManager")
    , max_connections_(max_connections)
    , websocket_handler_(std::make_unique<network::WebSocketHandler>(8080))
    , load_balancer_(std::make_unique<network::LoadBalancer>())
    , work_(std::make_unique<boost::asio::io_context::work>(io_context_)) {
}

void ConnectionManagerAgent::start() {
    LOG_INFO("Connection Manager Agent 시작");
    
    running_.store(true, std::memory_order_release);
    start_time_ = std::chrono::high_resolution_clock::now();
    
    // 워커 스레드 시작
    start_worker_threads();
    
    // WebSocket 핸들러 시작
    websocket_handler_->start();
    
    // 로드 밸런서 시작
    load_balancer_->start();
    
    update_metric("startup_time", std::chrono::duration<double>(
        std::chrono::high_resolution_clock::now() - start_time_).count());
}

void ConnectionManagerAgent::stop() {
    LOG_INFO("Connection Manager Agent 중지");
    
    running_.store(false, std::memory_order_release);
    
    // WebSocket 핸들러 중지
    websocket_handler_->stop();
    
    // 로드 밸런서 중지
    load_balancer_->stop();
    
    // 워커 스레드 중지
    stop_worker_threads();
    
    // 모든 연결 정리
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        connections_.clear();
        current_connections_.store(0, std::memory_order_release);
    }
}

bool ConnectionManagerAgent::handle_new_connection(const std::string& connection_id, 
                                                  const std::string& ip_address) {
    if (current_connections_.load(std::memory_order_acquire) >= max_connections_) {
        LOG_WARNING("최대 연결 수 초과: {}", max_connections_);
        update_metric("connection_rejected", 1.0);
        return false;
    }
    
    auto connection_info = std::make_unique<ConnectionInfo>();
    connection_info->connection_id = connection_id;
    connection_info->ip_address = ip_address;
    connection_info->connected_at = std::chrono::high_resolution_clock::now();
    connection_info->last_activity = connection_info->connected_at;
    
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        connections_[connection_id] = std::move(connection_info);
        current_connections_.fetch_add(1, std::memory_order_acq_rel);
    }
    
    LOG_INFO("새 연결 수락: {} from {}", connection_id, ip_address);
    update_metric("connections_total", current_connections_.load(std::memory_order_acquire));
    update_metric("connection_accepted", 1.0);
    
    return true;
}

void ConnectionManagerAgent::handle_disconnection(const std::string& connection_id) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    
    auto it = connections_.find(connection_id);
    if (it != connections_.end()) {
        connections_.erase(it);
        current_connections_.fetch_sub(1, std::memory_order_acq_rel);
        
        LOG_INFO("연결 해제: {}", connection_id);
        update_metric("connections_total", current_connections_.load(std::memory_order_acquire));
        update_metric("connection_disconnected", 1.0);
    }
}

void ConnectionManagerAgent::authenticate_connection(const std::string& connection_id, 
                                                    const std::string& user_id) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    
    auto it = connections_.find(connection_id);
    if (it != connections_.end()) {
        it->second->user_id = user_id;
        it->second->is_authenticated = true;
        
        LOG_INFO("연결 인증 완료: {} -> {}", connection_id, user_id);
        update_metric("authenticated_connections", 
                     std::count_if(connections_.begin(), connections_.end(),
                                  [](const auto& pair) { return pair.second->is_authenticated; }));
    }
}

void ConnectionManagerAgent::update_activity(const std::string& connection_id) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    
    auto it = connections_.find(connection_id);
    if (it != connections_.end()) {
        it->second->last_activity = std::chrono::high_resolution_clock::now();
    }
}

std::unordered_map<std::string, double> ConnectionManagerAgent::get_connection_stats() const {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    
    uint32_t total_connections = current_connections_.load(std::memory_order_acquire);
    uint32_t authenticated_connections = std::count_if(
        connections_.begin(), connections_.end(),
        [](const auto& pair) { return pair.second->is_authenticated; }
    );
    
    return {
        {"total_connections", static_cast<double>(total_connections)},
        {"authenticated_connections", static_cast<double>(authenticated_connections)},
        {"max_connections", static_cast<double>(max_connections_)},
        {"connection_utilization", static_cast<double>(total_connections) / max_connections_}
    };
}

std::optional<ConnectionInfo> ConnectionManagerAgent::get_connection_info(
    const std::string& connection_id) const {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    
    auto it = connections_.find(connection_id);
    if (it != connections_.end()) {
        return *it->second;
    }
    
    return std::nullopt;
}

void ConnectionManagerAgent::cleanup_inactive_connections(std::chrono::seconds timeout) {
    auto now = std::chrono::high_resolution_clock::now();
    std::vector<std::string> to_remove;
    
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        
        for (const auto& [connection_id, info] : connections_) {
            auto inactive_time = now - info->last_activity;
            if (inactive_time > timeout) {
                to_remove.push_back(connection_id);
            }
        }
    }
    
    for (const auto& connection_id : to_remove) {
        handle_disconnection(connection_id);
        LOG_INFO("비활성 연결 정리: {}", connection_id);
    }
    
    if (!to_remove.empty()) {
        update_metric("connections_cleaned", static_cast<double>(to_remove.size()));
    }
}

void ConnectionManagerAgent::start_worker_threads() {
    const size_t num_threads = std::thread::hardware_concurrency();
    worker_threads_.reserve(num_threads);
    
    for (size_t i = 0; i < num_threads; ++i) {
        worker_threads_.emplace_back([this]() {
            io_context_.run();
        });
    }
    
    LOG_INFO("워커 스레드 {}개 시작", num_threads);
}

void ConnectionManagerAgent::stop_worker_threads() {
    work_.reset();
    io_context_.stop();
    
    for (auto& thread : worker_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    worker_threads_.clear();
    LOG_INFO("워커 스레드 중지");
}

} // namespace mmorpg::agents::connection_manager
