#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
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
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#define close_socket close
typedef int SOCKET;
constexpr SOCKET INVALID_SOCKET = -1;
constexpr int SOCKET_ERROR      = -1;
#endif

namespace net {
using Ssize = std::ptrdiff_t;

class TcpSocket {
    SOCKET sock_    = INVALID_SOCKET;
    bool connected_ = false;

public:
    TcpSocket();
    explicit TcpSocket(SOCKET existing_socket);

    TcpSocket(const TcpSocket&)            = delete;
    TcpSocket& operator=(const TcpSocket&) = delete;

    TcpSocket(TcpSocket&& other) noexcept;
    TcpSocket& operator=(TcpSocket&& other) noexcept;

    ~TcpSocket();

    bool connect(const std::string& ip, std::uint16_t port);
    bool disconnect();

    [[nodiscard]] bool setTimeouts(unsigned send_timeout_sec, unsigned recv_timeout_sec) const;

    [[nodiscard]] bool sendBytes(const std::uint8_t* data, std::size_t size) const;
    [[nodiscard]] bool sendBytes(const std::vector<std::uint8_t>& data) const;

    [[nodiscard]] Ssize receiveBytes(std::uint8_t* buffer, std::size_t max_size = 64 * 1024) const;
    [[nodiscard]] std::vector<std::uint8_t> receiveBytes(std::size_t max_size = 64 * 1024) const;

    [[nodiscard]] bool isConnected() const;
    [[nodiscard]] std::string remoteIp() const;
};
}
