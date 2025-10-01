#include "network/load_balancer.hpp"
#include "common/logger.hpp"
#include <algorithm>
#include <random>
#include <thread>

namespace mmorpg::network {

LoadBalancer::LoadBalancer(LoadBalancingStrategy strategy)
    : strategy_(strategy) {
}

void LoadBalancer::start() {
    if (running_.load(std::memory_order_acquire)) {
        LOG_WARNING("LoadBalancer already running");
        return;
    }
    
    running_.store(true, std::memory_order_release);
    LOG_INFO("LoadBalancer started with strategy: {}", static_cast<int>(strategy_));
}

void LoadBalancer::stop() {
    if (!running_.load(std::memory_order_acquire)) {
        return;
    }
    
    running_.store(false, std::memory_order_release);
    
    if (health_check_thread_.joinable()) {
        health_check_thread_.join();
    }
    
    LOG_INFO("LoadBalancer stopped");
}

void LoadBalancer::add_server(const std::string& id, const std::string& host, uint16_t port, uint32_t max_connections) {
    std::lock_guard<std::mutex> lock(servers_mutex_);
    
    auto server = std::make_unique<ServerNode>();
    server->id = id;
    server->host = host;
    server->port = port;
    server->max_connections.store(max_connections, std::memory_order_release);
    server->last_health_check = std::chrono::steady_clock::now();
    
    servers_.push_back(std::move(server));
    LOG_INFO("Added server: {} ({}:{})", id, host, port);
}

void LoadBalancer::remove_server(const std::string& id) {
    std::lock_guard<std::mutex> lock(servers_mutex_);
    
    auto it = std::find_if(servers_.begin(), servers_.end(),
        [&id](const std::unique_ptr<ServerNode>& server) {
            return server->id == id;
        });
    
    if (it != servers_.end()) {
        LOG_INFO("Removed server: {}", id);
        servers_.erase(it);
    }
}

std::string LoadBalancer::select_server(const std::string& client_ip) {
    std::lock_guard<std::mutex> lock(servers_mutex_);
    
    if (servers_.empty()) {
        LOG_WARNING("No servers available for load balancing");
        return "";
    }
    
    // 헬시한 서버만 필터링
    std::vector<ServerNode*> healthy_servers;
    for (auto& server : servers_) {
        if (server->is_healthy.load(std::memory_order_acquire)) {
            healthy_servers.push_back(server.get());
        }
    }
    
    if (healthy_servers.empty()) {
        LOG_ERROR("No healthy servers available");
        return "";
    }
    
    switch (strategy_) {
        case LoadBalancingStrategy::ROUND_ROBIN:
            return select_round_robin();
        case LoadBalancingStrategy::LEAST_CONNECTIONS:
            return select_least_connections();
        case LoadBalancingStrategy::LEAST_LOAD:
            return select_least_load();
        case LoadBalancingStrategy::WEIGHTED_ROUND_ROBIN:
            return select_weighted_round_robin();
        case LoadBalancingStrategy::IP_HASH:
            return select_ip_hash(client_ip);
        default:
            return select_least_load();
    }
}

std::string LoadBalancer::select_round_robin() {
    if (servers_.empty()) {
        return "";
    }
    
    uint32_t index = round_robin_index_.fetch_add(1) % servers_.size();
    return servers_[index]->id;
}

std::string LoadBalancer::select_least_connections() {
    if (servers_.empty()) {
        return "";
    }
    
    auto min_server = std::min_element(servers_.begin(), servers_.end(),
        [](const std::unique_ptr<ServerNode>& a, const std::unique_ptr<ServerNode>& b) {
            return a->current_connections.load() < b->current_connections.load();
        });
    
    return (*min_server)->id;
}

std::string LoadBalancer::select_least_load() {
    if (servers_.empty()) {
        return "";
    }
    
    auto min_server = std::min_element(servers_.begin(), servers_.end(),
        [](const std::unique_ptr<ServerNode>& a, const std::unique_ptr<ServerNode>& b) {
            return a->get_load_score() < b->get_load_score();
        });
    
    return (*min_server)->id;
}

std::string LoadBalancer::select_weighted_round_robin() {
    if (servers_.empty()) {
        return "";
    }
    
    // 가중치는 max_connections의 역수로 계산
    std::vector<double> weights;
    double total_weight = 0.0;
    
    for (const auto& server : servers_) {
        double weight = 1.0 / static_cast<double>(server->max_connections.load());
        weights.push_back(weight);
        total_weight += weight;
    }
    
    // 가중치 기반 랜덤 선택
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, total_weight);
    
    double random_value = dis(gen);
    double current_weight = 0.0;
    
    for (size_t i = 0; i < servers_.size(); ++i) {
        current_weight += weights[i];
        if (random_value <= current_weight) {
            return servers_[i]->id;
        }
    }
    
    return servers_.back()->id;
}

std::string LoadBalancer::select_ip_hash(const std::string& client_ip) {
    if (servers_.empty()) {
        return "";
    }
    
    // IP 해시 계산
    std::hash<std::string> hasher;
    size_t hash_value = hasher(client_ip);
    uint32_t index = hash_value % servers_.size();
    
    return servers_[index]->id;
}

bool LoadBalancer::assign_connection(const std::string& server_id, const std::string& connection_id) {
    std::lock_guard<std::mutex> server_lock(servers_mutex_);
    std::lock_guard<std::mutex> connection_lock(connection_mutex_);
    
    // 서버 찾기
    auto server_it = std::find_if(servers_.begin(), servers_.end(),
        [&server_id](const std::unique_ptr<ServerNode>& server) {
            return server->id == server_id;
        });
    
    if (server_it == servers_.end()) {
        LOG_ERROR("Server not found: {}", server_id);
        return false;
    }
    
    auto* server = server_it->get();
    
    // 서버 용량 확인
    if (!server->can_accept_connection()) {
        LOG_WARNING("Server {} cannot accept more connections", server_id);
        return false;
    }
    
    // 연결 할당
    server->current_connections.fetch_add(1, std::memory_order_acq_rel);
    connection_to_server_[connection_id] = server_id;
    
    LOG_DEBUG("Assigned connection {} to server {}", connection_id, server_id);
    return true;
}

void LoadBalancer::release_connection(const std::string& server_id, const std::string& connection_id) {
    std::lock_guard<std::mutex> server_lock(servers_mutex_);
    std::lock_guard<std::mutex> connection_lock(connection_mutex_);
    
    // 서버 찾기
    auto server_it = std::find_if(servers_.begin(), servers_.end(),
        [&server_id](const std::unique_ptr<ServerNode>& server) {
            return server->id == server_id;
        });
    
    if (server_it != servers_.end()) {
        auto* server = server_it->get();
        server->current_connections.fetch_sub(1, std::memory_order_acq_rel);
    }
    
    // 연결 추적에서 제거
    connection_to_server_.erase(connection_id);
    
    LOG_DEBUG("Released connection {} from server {}", connection_id, server_id);
}

void LoadBalancer::update_server_status(const std::string& server_id, double cpu_usage, double memory_usage, bool is_healthy) {
    std::lock_guard<std::mutex> lock(servers_mutex_);
    
    auto server_it = std::find_if(servers_.begin(), servers_.end(),
        [&server_id](const std::unique_ptr<ServerNode>& server) {
            return server->id == server_id;
        });
    
    if (server_it != servers_.end()) {
        auto* server = server_it->get();
        server->cpu_usage.store(cpu_usage, std::memory_order_release);
        server->memory_usage.store(memory_usage, std::memory_order_release);
        server->is_healthy.store(is_healthy, std::memory_order_release);
        server->last_health_check = std::chrono::steady_clock::now();
        
        LOG_DEBUG("Updated server {} status: CPU={:.2f}%, Memory={:.2f}%, Healthy={}", 
                 server_id, cpu_usage, memory_usage, is_healthy);
    }
}

const ServerNode* LoadBalancer::get_server(const std::string& server_id) const {
    std::lock_guard<std::mutex> lock(servers_mutex_);
    
    auto server_it = std::find_if(servers_.begin(), servers_.end(),
        [&server_id](const std::unique_ptr<ServerNode>& server) {
            return server->id == server_id;
        });
    
    return (server_it != servers_.end()) ? server_it->get() : nullptr;
}

std::vector<ServerNode> LoadBalancer::get_all_servers() const {
    std::lock_guard<std::mutex> lock(servers_mutex_);
    
    std::vector<ServerNode> result;
    for (const auto& server : servers_) {
        result.push_back(*server);
    }
    
    return result;
}

void LoadBalancer::set_strategy(LoadBalancingStrategy strategy) {
    strategy_ = strategy;
    LOG_INFO("Load balancing strategy changed to: {}", static_cast<int>(strategy));
}

void LoadBalancer::start_health_check(std::chrono::seconds interval) {
    health_check_interval_ = interval;
    
    health_check_thread_ = std::thread([this]() {
        while (running_.load(std::memory_order_acquire)) {
            perform_health_check();
            std::this_thread::sleep_for(health_check_interval_);
        }
    });
    
    LOG_INFO("Health check started with interval: {} seconds", interval.count());
}

void LoadBalancer::perform_health_check() {
    std::lock_guard<std::mutex> lock(servers_mutex_);
    
    auto now = std::chrono::steady_clock::now();
    
    for (auto& server : servers_) {
        // 마지막 헬스 체크로부터 너무 오래 지났으면 비헬시로 표시
        auto time_since_check = now - server->last_health_check;
        if (time_since_check > std::chrono::minutes(5)) {
            server->is_healthy.store(false, std::memory_order_release);
            LOG_WARNING("Server {} marked as unhealthy due to no recent health check", server->id);
        }
    }
}

} // namespace mmorpg::network


