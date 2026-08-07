#include "net/tcp-socket.h"

#include "net/net-initializer.h"

#include <algorithm>
#include <limits>

namespace net {

TcpSocket::TcpSocket() {
    NetInitializer::ensureInitialized();
}

TcpSocket::TcpSocket(const SOCKET existing_socket) : sock_(existing_socket), connected_(true) {
    NetInitializer::ensureInitialized();
}

TcpSocket::TcpSocket(TcpSocket&& other) noexcept : sock_(other.sock_), connected_(other.connected_) {
    other.sock_      = INVALID_SOCKET;
    other.connected_ = false;
}

TcpSocket& TcpSocket::operator=(TcpSocket&& other) noexcept {
    if (this != &other) {
        disconnect();
        sock_            = other.sock_;
        connected_       = other.connected_;
        other.sock_      = INVALID_SOCKET;
        other.connected_ = false;
    }
    return *this;
}

TcpSocket::~TcpSocket() {
    disconnect();
}

bool TcpSocket::connect(const std::string& ip, const std::uint16_t port) {
    if (connected_)
        disconnect();

    sock_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_ == INVALID_SOCKET)
        return false;

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr) != 1) {
        close_socket(sock_);
        sock_ = INVALID_SOCKET;
        return false;
    }

    if (::connect(sock_, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == SOCKET_ERROR) {
        close_socket(sock_);
        sock_ = INVALID_SOCKET;
        return false;
    }

    connected_ = true;
    return true;
}

bool TcpSocket::disconnect() {
    if (sock_ != INVALID_SOCKET) {
        shutdown(sock_, SHUT_RDWR);
        close_socket(sock_);
        sock_ = INVALID_SOCKET;
    }
    connected_ = false;
    return true;
}

bool TcpSocket::setTimeouts(const unsigned send_timeout_sec, const unsigned recv_timeout_sec) const {
    if (sock_ == INVALID_SOCKET)
        return false;
#ifdef _WIN32
    const DWORD send_ms = send_timeout_sec * 1000u;
    const DWORD recv_ms = recv_timeout_sec * 1000u;
    if (setsockopt(sock_, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&send_ms), sizeof(send_ms)) != 0)
        return false;
    if (setsockopt(sock_, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&recv_ms), sizeof(recv_ms)) != 0)
        return false;
#else
    timeval send_tv{};
    send_tv.tv_sec  = static_cast<time_t>(send_timeout_sec);
    send_tv.tv_usec = 0;
    timeval recv_tv{};
    recv_tv.tv_sec  = static_cast<time_t>(recv_timeout_sec);
    recv_tv.tv_usec = 0;
    if (setsockopt(sock_, SOL_SOCKET, SO_SNDTIMEO, &send_tv, sizeof(send_tv)) != 0)
        return false;
    if (setsockopt(sock_, SOL_SOCKET, SO_RCVTIMEO, &recv_tv, sizeof(recv_tv)) != 0)
        return false;
#endif
    return true;
}

bool TcpSocket::sendBytes(const std::uint8_t* data, const std::size_t size) const {
    if (!connected_ || (size > 0 && data == nullptr))
        return false;

    constexpr auto k_max_chunk = static_cast<std::size_t>(std::numeric_limits<int>::max());
    std::size_t total_sent     = 0;
    while (total_sent < size) {
        const std::size_t remaining = size - total_sent;
        const int to_send           = static_cast<int>(std::min(remaining, k_max_chunk));
        const int bytes_sent        = send(sock_, reinterpret_cast<const char*>(data + total_sent), to_send, 0);
        if (bytes_sent == SOCKET_ERROR || bytes_sent == 0)
            return false;
        total_sent += static_cast<std::size_t>(bytes_sent);
    }
    return true;
}

bool TcpSocket::sendBytes(const std::vector<std::uint8_t>& data) const {
    return sendBytes(data.data(), data.size());
}

Ssize TcpSocket::receiveBytes(std::uint8_t* buffer, const std::size_t max_size) const {
    if (!connected_ || buffer == nullptr || max_size == 0)
        return -1;

    const std::size_t chunk  = std::min(max_size, static_cast<std::size_t>(std::numeric_limits<int>::max()));
    const int bytes_received = recv(sock_, reinterpret_cast<char*>(buffer), static_cast<int>(chunk), 0);

    if (bytes_received == SOCKET_ERROR)
        return -1;
    return static_cast<Ssize>(bytes_received);
}

std::vector<std::uint8_t> TcpSocket::receiveBytes(const std::size_t max_size) const {
    if (max_size == 0)
        return {};

    std::vector<std::uint8_t> buf(max_size);
    const Ssize received = receiveBytes(buf.data(), max_size);
    if (received <= 0)
        return {};
    buf.resize(static_cast<std::size_t>(received));
    return buf;
}

bool TcpSocket::isConnected() const {
    return connected_;
}

std::string TcpSocket::remoteIp() const {
    if (!connected_ || sock_ == INVALID_SOCKET)
        return {};

    sockaddr_in addr{};
#ifdef _WIN32
    int addr_size = sizeof(addr);
    if (getpeername(sock_, reinterpret_cast<sockaddr*>(&addr), &addr_size) == SOCKET_ERROR)
        return {};
#else
    socklen_t addr_size = sizeof(addr);
    if (getpeername(sock_, reinterpret_cast<sockaddr*>(&addr), &addr_size) == SOCKET_ERROR)
        return {};
#endif

    char ip_str[INET_ADDRSTRLEN]{};
    if (inet_ntop(AF_INET, &addr.sin_addr, ip_str, INET_ADDRSTRLEN) == nullptr)
        return {};
    return {ip_str};
}
}
