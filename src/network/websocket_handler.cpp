#include "network/websocket_handler.hpp"
#include "common/logger.hpp"
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/strand.hpp>
#include <iostream>

namespace mmorpg::network {

// WebSocketConnection 구현
WebSocketConnection::WebSocketConnection(tcp::socket socket, const std::string& connection_id)
    : connection_id_(connection_id)
    , ws_(std::move(socket)) {
}

void WebSocketConnection::perform_handshake() {
    ws_.async_accept(
        [self = shared_from_this()](beast::error_code ec) {
            self->on_handshake(ec);
        }
    );
}

void WebSocketConnection::on_handshake(beast::error_code ec) {
    if (ec) {
        LOG_ERROR("WebSocket handshake failed: {}", ec.message());
        return;
    }
    
    connected_.store(true, std::memory_order_release);
    LOG_INFO("WebSocket connection established: {}", connection_id_);
    
    start_reading();
}

void WebSocketConnection::start_reading() {
    ws_.async_read(
        buffer_,
        [self = shared_from_this()](beast::error_code ec, std::size_t bytes_transferred) {
            self->on_read(ec, bytes_transferred);
        }
    );
}

void WebSocketConnection::on_read(beast::error_code ec, std::size_t bytes_transferred) {
    if (ec) {
        if (ec == websocket::error::closed) {
            LOG_INFO("WebSocket connection closed: {}", connection_id_);
        } else {
            LOG_ERROR("WebSocket read error: {}", ec.message());
        }
        
        connected_.store(false, std::memory_order_release);
        if (close_handler_) {
            close_handler_();
        }
        return;
    }
    
    // 메시지 처리
    std::string message = beast::buffers_to_string(buffer_.data());
    buffer_.consume(buffer_.size());
    
    LOG_DEBUG("Received message from {}: {}", connection_id_, message);
    
    if (message_handler_) {
        message_handler_(message);
    }
    
    // 다음 메시지 읽기 계속
    start_reading();
}

void WebSocketConnection::send_message(const std::string& message) {
    if (!connected_.load(std::memory_order_acquire)) {
        LOG_WARNING("Attempted to send message to disconnected connection: {}", connection_id_);
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    ws_.async_write(
        net::buffer(message),
        [self = shared_from_this()](beast::error_code ec, std::size_t bytes_transferred) {
            self->on_write(ec, bytes_transferred);
        }
    );
}

void WebSocketConnection::on_write(beast::error_code ec, std::size_t bytes_transferred) {
    if (ec) {
        LOG_ERROR("WebSocket write error: {}", ec.message());
        connected_.store(false, std::memory_order_release);
        if (close_handler_) {
            close_handler_();
        }
    }
}

void WebSocketConnection::close() {
    if (connected_.load(std::memory_order_acquire)) {
        connected_.store(false, std::memory_order_release);
        
        beast::error_code ec;
        ws_.close(websocket::close_code::normal, ec);
        
        if (ec) {
            LOG_ERROR("WebSocket close error: {}", ec.message());
        }
        
        if (close_handler_) {
            close_handler_();
        }
    }
}

const std::string& WebSocketConnection::get_connection_id() const {
    return connection_id_;
}

bool WebSocketConnection::is_connected() const {
    return connected_.load(std::memory_order_acquire);
}

void WebSocketConnection::set_message_handler(std::function<void(const std::string&)> handler) {
    message_handler_ = std::move(handler);
}

void WebSocketConnection::set_close_handler(std::function<void()> handler) {
    close_handler_ = std::move(handler);
}

// WebSocketHandler 구현
WebSocketHandler::WebSocketHandler(uint16_t port)
    : port_(port)
    , acceptor_(io_context_) {
}

void WebSocketHandler::start() {
    if (running_.load(std::memory_order_acquire)) {
        LOG_WARNING("WebSocketHandler already running");
        return;
    }
    
    running_.store(true, std::memory_order_release);
    
    // 워커 스레드 시작
    const size_t num_threads = std::thread::hardware_concurrency();
    work_ = std::make_unique<net::io_context::work>(io_context_);
    
    for (size_t i = 0; i < num_threads; ++i) {
        worker_threads_.emplace_back([this]() {
            io_context_.run();
        });
    }
    
    // 서버 시작
    beast::error_code ec;
    tcp::endpoint endpoint(tcp::v4(), port_);
    
    acceptor_.open(endpoint.protocol(), ec);
    if (ec) {
        LOG_ERROR("Failed to open acceptor: {}", ec.message());
        return;
    }
    
    acceptor_.set_option(net::socket_base::reuse_address(true), ec);
    if (ec) {
        LOG_ERROR("Failed to set reuse address: {}", ec.message());
        return;
    }
    
    acceptor_.bind(endpoint, ec);
    if (ec) {
        LOG_ERROR("Failed to bind to endpoint: {}", ec.message());
        return;
    }
    
    acceptor_.listen(net::socket_base::max_listen_connections, ec);
    if (ec) {
        LOG_ERROR("Failed to listen: {}", ec.message());
        return;
    }
    
    LOG_INFO("WebSocket server started on port {}", port_);
    start_accept();
}

void WebSocketHandler::stop() {
    if (!running_.load(std::memory_order_acquire)) {
        return;
    }
    
    running_.store(false, std::memory_order_release);
    
    // 모든 연결 종료
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        for (auto& [id, connection] : connections_) {
            connection->close();
        }
        connections_.clear();
    }
    
    // 워커 스레드 중지
    work_.reset();
    io_context_.stop();
    
    for (auto& thread : worker_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    worker_threads_.clear();
    LOG_INFO("WebSocket server stopped");
}

void WebSocketHandler::start_accept() {
    acceptor_.async_accept(
        [this](beast::error_code ec, tcp::socket socket) {
            on_accept(ec, std::move(socket));
        }
    );
}

void WebSocketHandler::on_accept(beast::error_code ec, tcp::socket socket) {
    if (ec) {
        LOG_ERROR("Accept error: {}", ec.message());
        return;
    }
    
    // 새 연결 ID 생성
    std::string connection_id = "conn_" + std::to_string(next_connection_id_.fetch_add(1));
    
    // WebSocket 연결 생성
    auto connection = std::make_shared<WebSocketConnection>(std::move(socket), connection_id);
    
    // 핸들러 설정
    connection->set_message_handler(
        [this, connection_id](const std::string& message) {
            on_message(connection_id, message);
        }
    );
    
    connection->set_close_handler(
        [this, connection_id]() {
            on_disconnection(connection_id);
        }
    );
    
    // 연결 저장
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        connections_[connection_id] = connection;
    }
    
    // 연결 핸드셰이크 시작
    connection->perform_handshake();
    
    // 연결 핸들러 호출
    on_connection(connection_id);
    
    // 다음 연결 대기
    start_accept();
}

void WebSocketHandler::send_to_connection(const std::string& connection_id, const std::string& message) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    
    auto it = connections_.find(connection_id);
    if (it != connections_.end()) {
        it->second->send_message(message);
    } else {
        LOG_WARNING("Connection not found: {}", connection_id);
    }
}

void WebSocketHandler::broadcast(const std::string& message) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    
    for (auto& [id, connection] : connections_) {
        if (connection->is_connected()) {
            connection->send_message(message);
        }
    }
}

size_t WebSocketHandler::get_connection_count() const {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    return connections_.size();
}

bool WebSocketHandler::has_connection(const std::string& connection_id) const {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    return connections_.find(connection_id) != connections_.end();
}

void WebSocketHandler::set_message_handler(MessageHandler handler) {
    message_handler_ = std::move(handler);
}

void WebSocketHandler::set_connection_handler(ConnectionHandler handler) {
    connection_handler_ = std::move(handler);
}

void WebSocketHandler::set_disconnection_handler(ConnectionHandler handler) {
    disconnection_handler_ = std::move(handler);
}

void WebSocketHandler::on_message(const std::string& connection_id, const std::string& message) {
    if (message_handler_) {
        message_handler_(connection_id, message);
    }
}

void WebSocketHandler::on_connection(const std::string& connection_id) {
    LOG_INFO("New WebSocket connection: {}", connection_id);
    
    if (connection_handler_) {
        connection_handler_(connection_id);
    }
}

void WebSocketHandler::on_disconnection(const std::string& connection_id) {
    LOG_INFO("WebSocket connection disconnected: {}", connection_id);
    
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        connections_.erase(connection_id);
    }
    
    if (disconnection_handler_) {
        disconnection_handler_(connection_id);
    }
}

} // namespace mmorpg::network


