#include "net/tcp-server.h"

#include "net/core/net-initializer.h"

namespace net {
TcpServer::TcpServer() {
    NetInitializer::ensureInitialized();
}

TcpServer::TcpServer(TcpServer&& other) noexcept
    : server_socket_(other.server_socket_), listening_(other.listening_), options_(other.options_) {
    other.server_socket_ = INVALID_SOCKET;
    other.listening_     = false;
}

TcpServer::~TcpServer() {
    close();
}

TcpServer& TcpServer::operator=(TcpServer&& other) noexcept {
    if (this != &other) {
        close();
        server_socket_       = other.server_socket_;
        listening_           = other.listening_;
        options_             = other.options_;
        other.server_socket_ = INVALID_SOCKET;
        other.listening_     = false;
    }
    return *this;
}

bool TcpServer::listen(const std::string& ip, const std::uint16_t port, const int backlog) {
    if (listening_)
        close();

    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_ == INVALID_SOCKET)
        return false;

    if (options_.reuse_address) {
        constexpr int reuse = 1;
#ifdef _WIN32
        setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse), sizeof(reuse));
#else
        setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
#endif
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr) != 1) {
        close_socket(server_socket_);
        server_socket_ = INVALID_SOCKET;
        return false;
    }

    if (bind(server_socket_, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == SOCKET_ERROR) {
        close_socket(server_socket_);
        server_socket_ = INVALID_SOCKET;
        return false;
    }
    if (::listen(server_socket_, backlog) == SOCKET_ERROR) {
        close_socket(server_socket_);
        server_socket_ = INVALID_SOCKET;
        return false;
    }

    listening_ = true;
    return true;
}

std::unique_ptr<TcpSocket> TcpServer::acceptConnection() const {
    if (!listening_)
        return nullptr;

    sockaddr_in client_addr{};
#ifdef _WIN32
    int client_len = sizeof(client_addr);
#else
    socklen_t client_len = sizeof(client_addr);
#endif
    const SOCKET client_socket = accept(server_socket_, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
    if (client_socket == INVALID_SOCKET)
        return nullptr;

    auto socket = std::make_unique<TcpSocket>(client_socket);
    static_cast<void>(socket->setOptions(options_.accepted));
    return socket;
}

void TcpServer::close() {
    if (server_socket_ != INVALID_SOCKET) {
        close_socket(server_socket_);
        server_socket_ = INVALID_SOCKET;
    }
    listening_ = false;
}

void TcpServer::setOptions(const ServerOptions& options) {
    options_ = options;
}

const ServerOptions& TcpServer::options() const {
    return options_;
}

bool TcpServer::isListening() const {
    return listening_;
}
}
