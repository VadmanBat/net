#pragma once

#include <memory>
#include <net/tcp-socket.h>
#include <string>

namespace net {

class TcpServer {
    SOCKET server_socket_ = INVALID_SOCKET;
    bool listening_       = false;

public:
    TcpServer();
    ~TcpServer();

    TcpServer(const TcpServer&)            = delete;
    TcpServer& operator=(const TcpServer&) = delete;

    TcpServer(TcpServer&& other) noexcept;
    TcpServer& operator=(TcpServer&& other) noexcept;

    bool listen(const std::string& ip, std::uint16_t port, int backlog = 5);
    [[nodiscard]] std::unique_ptr<TcpSocket> acceptConnection() const;
    void close();
    [[nodiscard]] bool isListening() const;
};
}
