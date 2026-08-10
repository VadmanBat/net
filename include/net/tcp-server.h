#pragma once

#include "net/server/server-options.h"
#include "net/tcp-socket.h"

#include <memory>
#include <string>

namespace net {
class TcpServer {
    SOCKET server_socket_ = INVALID_SOCKET;
    bool listening_       = false;
    ServerOptions options_{};

public:
    TcpServer();
    TcpServer(const TcpServer&) = delete;
    TcpServer(TcpServer&& other) noexcept;

    ~TcpServer();

    TcpServer& operator=(const TcpServer&) = delete;
    TcpServer& operator=(TcpServer&& other) noexcept;

    bool listen(const std::string& ip, std::uint16_t port, int backlog = 5);
    [[nodiscard]] std::unique_ptr<TcpSocket> acceptConnection() const;
    void close();

    void setOptions(const ServerOptions& options);
    [[nodiscard]] const ServerOptions& options() const;

    [[nodiscard]] bool isListening() const;
};
}
