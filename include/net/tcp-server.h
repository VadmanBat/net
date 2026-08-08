#pragma once

#include <memory>
#include <string>

#include <net/tcp-socket.h>

namespace net {

/// Listen + defaults for each accepted peer. One object instead of many server fields.
struct ServerOptions {
    bool reuse_address = true;
    SocketOptions accepted{};
};

class TcpServer {
private:
    SOCKET server_socket_ = INVALID_SOCKET;
    bool listening_       = false;
    ServerOptions options_{};

public:
    TcpServer();
    ~TcpServer();

    TcpServer(const TcpServer&)            = delete;
    TcpServer& operator=(const TcpServer&) = delete;

    TcpServer(TcpServer&& other) noexcept;
    TcpServer& operator=(TcpServer&& other) noexcept;

    void setOptions(const ServerOptions& options);
    [[nodiscard]] const ServerOptions& options() const;

    bool listen(const std::string& ip, std::uint16_t port, int backlog = 5);
    [[nodiscard]] std::unique_ptr<TcpSocket> acceptConnection() const;
    void close();
    [[nodiscard]] bool isListening() const;
};

} // namespace net
