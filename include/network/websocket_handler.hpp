#pragma once

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <memory>
#include <unordered_map>
#include <atomic>
#include <mutex>
#include <functional>
#include <string>

namespace mmorpg::network {

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

/**
 * @brief WebSocket 연결을 나타내는 클래스
 */
class WebSocketConnection {
public:
    using Ptr = std::shared_ptr<WebSocketConnection>;
    
    WebSocketConnection(tcp::socket socket, const std::string& connection_id);
    ~WebSocketConnection() = default;
    
    // 복사 및 이동 방지
    WebSocketConnection(const WebSocketConnection&) = delete;
    WebSocketConnection& operator=(const WebSocketConnection&) = delete;
    WebSocketConnection(WebSocketConnection&&) = delete;
    WebSocketConnection& operator=(WebSocketConnection&&) = delete;
    
    /**
     * @brief WebSocket 핸드셰이크 수행
     */
    void perform_handshake();
    
    /**
     * @brief 메시지 읽기 시작
     */
    void start_reading();
    
    /**
     * @brief 메시지 전송
     */
    void send_message(const std::string& message);
    
    /**
     * @brief 연결 종료
     */
    void close();
    
    /**
     * @brief 연결 ID 반환
     */
    const std::string& get_connection_id() const;
    
    /**
     * @brief 연결 상태 확인
     */
    bool is_connected() const;
    
    /**
     * @brief 메시지 핸들러 설정
     */
    void set_message_handler(std::function<void(const std::string&)> handler);
    
    /**
     * @brief 연결 종료 핸들러 설정
     */
    void set_close_handler(std::function<void()> handler);

private:
    void on_handshake(beast::error_code ec);
    void on_read(beast::error_code ec, std::size_t bytes_transferred);
    void on_write(beast::error_code ec, std::size_t bytes_transferred);
    void on_close(beast::error_code ec);
    
    std::string connection_id_;
    websocket::stream<tcp::socket> ws_;
    beast::flat_buffer buffer_;
    std::atomic<bool> connected_{false};
    
    std::function<void(const std::string&)> message_handler_;
    std::function<void()> close_handler_;
    
    mutable std::mutex mutex_;
};

/**
 * @brief WebSocket 서버 핸들러
 */
class WebSocketHandler {
public:
    using ConnectionPtr = WebSocketConnection::Ptr;
    using MessageHandler = std::function<void(const std::string&, const std::string&)>;
    using ConnectionHandler = std::function<void(const std::string&)>;
    
    explicit WebSocketHandler(uint16_t port = 8080);
    ~WebSocketHandler() = default;
    
    /**
     * @brief 서버 시작
     */
    void start();
    
    /**
     * @brief 서버 중지
     */
    void stop();
    
    /**
     * @brief 특정 연결에 메시지 전송
     */
    void send_to_connection(const std::string& connection_id, const std::string& message);
    
    /**
     * @brief 모든 연결에 브로드캐스트
     */
    void broadcast(const std::string& message);
    
    /**
     * @brief 연결 수 반환
     */
    size_t get_connection_count() const;
    
    /**
     * @brief 특정 연결 존재 여부 확인
     */
    bool has_connection(const std::string& connection_id) const;
    
    /**
     * @brief 메시지 핸들러 설정
     */
    void set_message_handler(MessageHandler handler);
    
    /**
     * @brief 연결 핸들러 설정
     */
    void set_connection_handler(ConnectionHandler handler);
    
    /**
     * @brief 연결 해제 핸들러 설정
     */
    void set_disconnection_handler(ConnectionHandler handler);

private:
    void start_accept();
    void on_accept(beast::error_code ec, tcp::socket socket);
    void on_message(const std::string& connection_id, const std::string& message);
    void on_connection(const std::string& connection_id);
    void on_disconnection(const std::string& connection_id);
    
    uint16_t port_;
    net::io_context io_context_;
    tcp::acceptor acceptor_;
    std::unique_ptr<net::io_context::work> work_;
    std::vector<std::thread> worker_threads_;
    
    mutable std::mutex connections_mutex_;
    std::unordered_map<std::string, ConnectionPtr> connections_;
    
    MessageHandler message_handler_;
    ConnectionHandler connection_handler_;
    ConnectionHandler disconnection_handler_;
    
    std::atomic<bool> running_{false};
    std::atomic<uint64_t> next_connection_id_{1};
};

} // namespace mmorpg::network
