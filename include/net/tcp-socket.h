#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
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

struct SocketOptions {
    std::optional<bool> no_delay;
    std::optional<bool> keep_alive;
    std::optional<std::size_t> send_buffer_size;
    std::optional<std::size_t> recv_buffer_size;
    std::optional<unsigned> send_timeout_sec;
    std::optional<unsigned> recv_timeout_sec;
};

struct SocketStatus {
    bool has_socket     = false;
    bool flag_connected = false;
    bool live_connected = false;

    std::string local_ip;
    std::uint16_t local_port = 0;
    std::string remote_ip;
    std::uint16_t remote_port = 0;

    bool no_delay                = false;
    bool keep_alive              = false;
    std::size_t send_buffer_size = 0;
    std::size_t recv_buffer_size = 0;

    int last_os_error = 0;
    std::string last_os_error_text;

    std::string text;
};

class TcpSocket {
    SOCKET sock_               = INVALID_SOCKET;
    mutable bool connected_    = false;
    mutable int last_os_error_ = 0;

    [[nodiscard]] bool set_sock_opt_int(int level, int opt_name, int value) const;
    [[nodiscard]] bool get_sock_opt_int(int level, int opt_name, int& value) const;

    void note_os_error() const;
    void clear_os_error() const;

public:
    TcpSocket();
    explicit TcpSocket(SOCKET existing_socket);
    TcpSocket(const TcpSocket&) = delete;
    TcpSocket(TcpSocket&& other) noexcept;

    TcpSocket& operator=(const TcpSocket&) = delete;
    TcpSocket& operator=(TcpSocket&& other) noexcept;

    ~TcpSocket();

    bool connect(const std::string& ip, std::uint16_t port);
    bool disconnect();

    [[nodiscard]] bool sendBytes(const std::uint8_t* data, std::size_t size) const;
    [[nodiscard]] bool sendBytes(const std::vector<std::uint8_t>& data) const;

    [[nodiscard]] Ssize receiveBytes(std::uint8_t* buffer, std::size_t max_size = 64 * 1024) const;
    [[nodiscard]] std::vector<std::uint8_t> receiveBytes(std::size_t max_size = 64 * 1024) const;

    [[nodiscard]] bool setNoDelay(bool enabled) const;
    [[nodiscard]] bool setKeepAlive(bool enabled) const;
    [[nodiscard]] bool setSendBufferSize(std::size_t bytes) const;
    [[nodiscard]] bool setRecvBufferSize(std::size_t bytes) const;
    [[nodiscard]] bool setTimeouts(unsigned send_timeout_sec, unsigned recv_timeout_sec) const;
    [[nodiscard]] bool setOptions(const SocketOptions& options) const;

    [[nodiscard]] bool isConnected() const;

    [[nodiscard]] bool noDelay() const;
    [[nodiscard]] bool keepAlive() const;
    [[nodiscard]] std::size_t sendBufferSize() const;
    [[nodiscard]] std::size_t recvBufferSize() const;
    [[nodiscard]] int lastOsError() const noexcept;
    [[nodiscard]] SocketStatus status() const;
    [[nodiscard]] std::string statusText() const;
    [[nodiscard]] std::string remoteIp() const;
};
}
