#include "net/tcp-socket.h"

#ifndef _WIN32
#include <cerrno>
#include <netinet/tcp.h>
#include <sys/select.h>
#endif

namespace net {
namespace {
bool socket_error_is_would_block() {
#ifdef _WIN32
    const int err = WSAGetLastError();
    return err == WSAEWOULDBLOCK || err == WSAEINTR;
#else
    return errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR;
#endif
}
}

bool TcpSocket::get_sock_opt_int(const int level, const int opt_name, int& value) const {
    if (sock_ == INVALID_SOCKET)
        return false;
#ifdef _WIN32
    int len = sizeof(value);
    return getsockopt(sock_, level, opt_name, reinterpret_cast<char*>(&value), &len) == 0;
#else
    socklen_t len = sizeof(value);
    return getsockopt(sock_, level, opt_name, &value, &len) == 0;
#endif
}

bool TcpSocket::isConnected() const {
    if (sock_ == INVALID_SOCKET) {
        connected_ = false;
        return false;
    }

    sockaddr_in addr{};
#ifdef _WIN32
    int addr_len = sizeof(addr);
    if (getpeername(sock_, reinterpret_cast<sockaddr*>(&addr), &addr_len) == SOCKET_ERROR) {
        connected_ = false;
        return false;
    }
#else
    socklen_t addr_len = sizeof(addr);
    if (getpeername(sock_, reinterpret_cast<sockaddr*>(&addr), &addr_len) == SOCKET_ERROR) {
        connected_ = false;
        return false;
    }
#endif

    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(sock_, &read_fds);
    timeval tv{};
    tv.tv_sec  = 0;
    tv.tv_usec = 0;

#ifdef _WIN32
    const int ready = select(0, &read_fds, nullptr, nullptr, &tv);
#else
    const int ready = select(static_cast<int>(sock_) + 1, &read_fds, nullptr, nullptr, &tv);
#endif
    if (ready < 0) {
        connected_ = false;
        return false;
    }
    if (ready == 0) {
        connected_ = true;
        return true;
    }

    char byte        = 0;
    const int peeked = recv(sock_, &byte, 1, MSG_PEEK);
    if (peeked == 0) {
        connected_ = false;
        return false;
    }
    if (peeked > 0) {
        connected_ = true;
        return true;
    }

    if (socket_error_is_would_block()) {
        connected_ = true;
        return true;
    }

    connected_ = false;
    return false;
}

bool TcpSocket::noDelay() const {
    int value = 0;
    if (!get_sock_opt_int(IPPROTO_TCP, TCP_NODELAY, value))
        return false;
    return value != 0;
}

bool TcpSocket::keepAlive() const {
    int value = 0;
    if (!get_sock_opt_int(SOL_SOCKET, SO_KEEPALIVE, value))
        return false;
    return value != 0;
}

std::size_t TcpSocket::sendBufferSize() const {
    int value = 0;
    if (!get_sock_opt_int(SOL_SOCKET, SO_SNDBUF, value) || value < 0)
        return 0;
    return static_cast<std::size_t>(value);
}

std::size_t TcpSocket::recvBufferSize() const {
    int value = 0;
    if (!get_sock_opt_int(SOL_SOCKET, SO_RCVBUF, value) || value < 0)
        return 0;
    return static_cast<std::size_t>(value);
}

std::string TcpSocket::remoteIp() const {
    if (sock_ == INVALID_SOCKET)
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
