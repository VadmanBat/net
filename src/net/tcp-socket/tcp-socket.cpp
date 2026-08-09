#include "net/tcp-socket.h"

#include "net/net-initializer.h"

#ifndef _WIN32
#include <cerrno>
#endif

namespace net {
TcpSocket::TcpSocket() {
    NetInitializer::ensureInitialized();
}

TcpSocket::TcpSocket(const SOCKET existing_socket) : sock_(existing_socket), connected_(true) {
    NetInitializer::ensureInitialized();
}

TcpSocket::TcpSocket(TcpSocket&& other) noexcept
    : sock_(other.sock_), connected_(other.connected_), last_os_error_(other.last_os_error_) {
    other.sock_          = INVALID_SOCKET;
    other.connected_     = false;
    other.last_os_error_ = 0;
}

TcpSocket::~TcpSocket() {
    disconnect();
}

TcpSocket& TcpSocket::operator=(TcpSocket&& other) noexcept {
    if (this != &other) {
        disconnect();
        sock_                = other.sock_;
        connected_           = other.connected_;
        last_os_error_       = other.last_os_error_;
        other.sock_          = INVALID_SOCKET;
        other.connected_     = false;
        other.last_os_error_ = 0;
    }
    return *this;
}

bool TcpSocket::connect(const std::string& ip, const std::uint16_t port) {
    if (connected_)
        disconnect();

    sock_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_ == INVALID_SOCKET) {
        note_os_error();
        return false;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr) != 1) {
        close_socket(sock_);
        sock_ = INVALID_SOCKET;
#ifdef _WIN32
        last_os_error_ = WSAEINVAL;
#else
        last_os_error_ = EINVAL;
#endif
        return false;
    }

    if (::connect(sock_, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == SOCKET_ERROR) {
        note_os_error();
        close_socket(sock_);
        sock_ = INVALID_SOCKET;
        return false;
    }

    connected_ = true;
    clear_os_error();
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
}
