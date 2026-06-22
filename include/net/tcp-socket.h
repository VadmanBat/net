#pragma once

#include <cstdint>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <vector>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#ifdef byte
#undef byte
#endif
#define close_socket closesocket
#define SHUT_RDWR SD_BOTH
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#define close_socket close
#define SHUT_RDWR SHUT_RDWR
typedef int SOCKET;
constexpr SOCKET INVALID_SOCKET = -1;
constexpr int SOCKET_ERROR      = -1;
#endif

namespace net {
class TcpSocket {
    SOCKET sock    = INVALID_SOCKET;
    bool connected = false;

public:
    TcpSocket();
    explicit TcpSocket(SOCKET existing_socket);

    TcpSocket(const TcpSocket&)            = delete;
    TcpSocket& operator=(const TcpSocket&) = delete;
    ~TcpSocket();

    bool connect(const std::string& ip, uint16_t port);
    bool disconnect();

    [[nodiscard]] bool sendBytes(const uint8_t* data, size_t size) const;
    [[nodiscard]] bool sendBytes(const std::vector<uint8_t>& data) const;

    [[nodiscard]] ssize_t receiveBytes(uint8_t* buffer, size_t max_size = 1024 * 1024) const;
    [[nodiscard]] std::vector<uint8_t> receiveBytes(size_t max_size = 1024 * 1024) const;

    [[nodiscard]] bool isConnected() const;
    [[nodiscard]] std::string getRemoteIp() const;
};
}