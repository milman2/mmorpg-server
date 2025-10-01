#include <gtest/gtest.h>
#include "agents/connection_manager/connection_manager.hpp"
#include "common/logger.hpp"
#include <thread>
#include <chrono>

namespace mmorpg::tests {

class ConnectionManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        mmorpg::common::Logger::initialize("test.log");
        connection_manager = std::make_unique<mmorpg::agents::connection_manager::ConnectionManagerAgent>(100);
    }
    
    void TearDown() override {
        if (connection_manager) {
            connection_manager->stop();
        }
        connection_manager.reset();
    }
    
    std::unique_ptr<mmorpg::agents::connection_manager::ConnectionManagerAgent> connection_manager;
};

TEST_F(ConnectionManagerTest, BasicInitialization) {
    EXPECT_FALSE(connection_manager->is_running());
    EXPECT_EQ(connection_manager->get_agent_id(), "ConnectionManager");
}

TEST_F(ConnectionManagerTest, StartAndStop) {
    connection_manager->start();
    EXPECT_TRUE(connection_manager->is_running());
    
    // 잠시 대기
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    connection_manager->stop();
    EXPECT_FALSE(connection_manager->is_running());
}

TEST_F(ConnectionManagerTest, HandleNewConnection) {
    connection_manager->start();
    
    // 새 연결 처리
    bool result = connection_manager->handle_new_connection("test_conn_1", "127.0.0.1");
    EXPECT_TRUE(result);
    
    // 중복 연결 시도
    result = connection_manager->handle_new_connection("test_conn_1", "127.0.0.1");
    EXPECT_FALSE(result); // 이미 존재하는 연결 ID
    
    connection_manager->stop();
}

TEST_F(ConnectionManagerTest, HandleDisconnection) {
    connection_manager->start();
    
    // 연결 생성
    connection_manager->handle_new_connection("test_conn_1", "127.0.0.1");
    
    // 연결 해제
    connection_manager->handle_disconnection("test_conn_1");
    
    // 다시 같은 ID로 연결 시도
    bool result = connection_manager->handle_new_connection("test_conn_1", "127.0.0.1");
    EXPECT_TRUE(result);
    
    connection_manager->stop();
}

TEST_F(ConnectionManagerTest, AuthenticateConnection) {
    connection_manager->start();
    
    // 연결 생성
    connection_manager->handle_new_connection("test_conn_1", "127.0.0.1");
    
    // 인증
    connection_manager->authenticate_connection("test_conn_1", "user_123");
    
    // 연결 정보 확인
    auto conn_info = connection_manager->get_connection_info("test_conn_1");
    ASSERT_TRUE(conn_info.has_value());
    EXPECT_EQ(conn_info->user_id, "user_123");
    EXPECT_TRUE(conn_info->is_authenticated);
    
    connection_manager->stop();
}

TEST_F(ConnectionManagerTest, UpdateActivity) {
    connection_manager->start();
    
    // 연결 생성
    connection_manager->handle_new_connection("test_conn_1", "127.0.0.1");
    
    auto conn_info_before = connection_manager->get_connection_info("test_conn_1");
    ASSERT_TRUE(conn_info_before.has_value());
    
    // 잠시 대기
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    // 활동 업데이트
    connection_manager->update_activity("test_conn_1");
    
    auto conn_info_after = connection_manager->get_connection_info("test_conn_1");
    ASSERT_TRUE(conn_info_after.has_value());
    
    // 마지막 활동 시간이 업데이트되었는지 확인
    EXPECT_GT(conn_info_after->last_activity, conn_info_before->last_activity);
    
    connection_manager->stop();
}

TEST_F(ConnectionManagerTest, ConnectionStats) {
    connection_manager->start();
    
    // 초기 통계
    auto stats = connection_manager->get_connection_stats();
    EXPECT_EQ(stats["total_connections"], 0);
    EXPECT_EQ(stats["authenticated_connections"], 0);
    EXPECT_EQ(stats["max_connections"], 100);
    EXPECT_EQ(stats["connection_utilization"], 0.0);
    
    // 연결 추가
    connection_manager->handle_new_connection("test_conn_1", "127.0.0.1");
    connection_manager->handle_new_connection("test_conn_2", "127.0.0.1");
    
    stats = connection_manager->get_connection_stats();
    EXPECT_EQ(stats["total_connections"], 2);
    EXPECT_EQ(stats["authenticated_connections"], 0);
    EXPECT_EQ(stats["connection_utilization"], 0.02); // 2/100
    
    // 인증
    connection_manager->authenticate_connection("test_conn_1", "user_123");
    
    stats = connection_manager->get_connection_stats();
    EXPECT_EQ(stats["total_connections"], 2);
    EXPECT_EQ(stats["authenticated_connections"], 1);
    
    connection_manager->stop();
}

TEST_F(ConnectionManagerTest, MaxConnectionsLimit) {
    connection_manager->start();
    
    // 최대 연결 수까지 연결 생성
    for (int i = 0; i < 100; ++i) {
        std::string conn_id = "test_conn_" + std::to_string(i);
        bool result = connection_manager->handle_new_connection(conn_id, "127.0.0.1");
        EXPECT_TRUE(result);
    }
    
    // 최대 연결 수 초과 시도
    bool result = connection_manager->handle_new_connection("test_conn_100", "127.0.0.1");
    EXPECT_FALSE(result);
    
    auto stats = connection_manager->get_connection_stats();
    EXPECT_EQ(stats["total_connections"], 100);
    EXPECT_EQ(stats["connection_utilization"], 1.0);
    
    connection_manager->stop();
}

TEST_F(ConnectionManagerTest, CleanupInactiveConnections) {
    connection_manager->start();
    
    // 연결 생성
    connection_manager->handle_new_connection("test_conn_1", "127.0.0.1");
    
    auto stats_before = connection_manager->get_connection_stats();
    EXPECT_EQ(stats_before["total_connections"], 1);
    
    // 비활성 연결 정리 (1초 타임아웃)
    connection_manager->cleanup_inactive_connections(std::chrono::seconds(1));
    
    // 아직 정리되지 않아야 함 (방금 생성했으므로)
    auto stats_after = connection_manager->get_connection_stats();
    EXPECT_EQ(stats_after["total_connections"], 1);
    
    // 1초 대기 후 정리
    std::this_thread::sleep_for(std::chrono::seconds(2));
    connection_manager->cleanup_inactive_connections(std::chrono::seconds(1));
    
    auto stats_final = connection_manager->get_connection_stats();
    EXPECT_EQ(stats_final["total_connections"], 0);
    
    connection_manager->stop();
}

} // namespace mmorpg::tests


