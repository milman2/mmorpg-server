#include "common/base_agent.hpp"
#include <stdexcept>

namespace mmorpg::common {

BaseAgent::BaseAgent(const std::string& agent_id)
    : agent_id_(agent_id.empty() ? typeid(*this).name() : agent_id)
    , start_time_(Clock::now()) {
}

bool BaseAgent::is_running() const noexcept {
    return running_.load(std::memory_order_acquire);
}

const std::string& BaseAgent::get_agent_id() const noexcept {
    return agent_id_;
}

BaseAgent::Duration BaseAgent::get_uptime() const {
    if (!running_.load(std::memory_order_acquire)) {
        return Duration::zero();
    }
    return Clock::now() - start_time_;
}

void BaseAgent::update_metric(const std::string& key, double value) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    metrics_[key] = value;
}

double BaseAgent::get_metric(const std::string& key, double default_value) const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    auto it = metrics_.find(key);
    return (it != metrics_.end()) ? it->second : default_value;
}

const std::unordered_map<std::string, double>& BaseAgent::get_metrics() const {
    return metrics_;
}

std::unordered_map<std::string, std::string> BaseAgent::health_check() const {
    std::unordered_map<std::string, std::string> health;
    
    health["agent_id"] = agent_id_;
    health["is_running"] = running_.load(std::memory_order_acquire) ? "true" : "false";
    health["uptime_seconds"] = std::to_string(get_uptime().count());
    
    // 메트릭 추가
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    for (const auto& [key, value] : metrics_) {
        health["metric_" + key] = std::to_string(value);
    }
    
    return health;
}

} // namespace mmorpg::common
