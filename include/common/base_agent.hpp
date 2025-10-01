#pragma once

#include <string>
#include <chrono>
#include <atomic>
#include <memory>
#include <unordered_map>
#include <functional>
#include <mutex>
#include <typeinfo>

namespace mmorpg::common {

/**
 * @brief 모든 에이전트의 기본 클래스
 * 
 * 공통 기능과 인터페이스를 제공합니다.
 */
class BaseAgent {
public:
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = std::chrono::time_point<Clock>;
    using Duration = std::chrono::duration<double>;

    explicit BaseAgent(const std::string& agent_id = "");
    virtual ~BaseAgent() = default;

    // 복사 및 이동 방지
    BaseAgent(const BaseAgent&) = delete;
    BaseAgent& operator=(const BaseAgent&) = delete;
    BaseAgent(BaseAgent&&) = delete;
    BaseAgent& operator=(BaseAgent&&) = delete;

    /**
     * @brief 에이전트 시작
     */
    virtual void start() = 0;

    /**
     * @brief 에이전트 중지
     */
    virtual void stop() = 0;

    /**
     * @brief 에이전트 상태 확인
     */
    virtual bool is_running() const noexcept;

    /**
     * @brief 에이전트 ID 반환
     */
    const std::string& get_agent_id() const noexcept;

    /**
     * @brief 가동 시간 반환 (초)
     */
    Duration get_uptime() const;

    /**
     * @brief 메트릭 업데이트
     */
    void update_metric(const std::string& key, double value);

    /**
     * @brief 메트릭 조회
     */
    double get_metric(const std::string& key, double default_value = 0.0) const;

    /**
     * @brief 모든 메트릭 반환
     */
    const std::unordered_map<std::string, double>& get_metrics() const;

    /**
     * @brief 헬스 체크
     */
    virtual std::unordered_map<std::string, std::string> health_check() const;

protected:
    std::string agent_id_;
    std::atomic<bool> running_{false};
    TimePoint start_time_;
    mutable std::unordered_map<std::string, double> metrics_;
    mutable std::mutex metrics_mutex_;
};

} // namespace mmorpg::common
