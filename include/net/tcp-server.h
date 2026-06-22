#pragma once

#include <net/tcp-socket.h>

namespace net {
class TcpServer {
    SOCKET server_socket = INVALID_SOCKET;
    bool listening       = false;

public:
    TcpServer();
    ~TcpServer();

    bool listen(const std::string& ip, uint16_t port, int backlog = 5);
    [[nodiscard]] std::unique_ptr<TcpSocket> acceptConnection() const;
    void close();
    [[nodiscard]] bool isListening() const;
};
}