#include "net/tcp-socket.h"

#include <limits>

#ifndef _WIN32
#include <netinet/tcp.h>
#endif

namespace net {

bool TcpSocket::set_sock_opt_int(const int level, const int opt_name, const int value) const {
    if (sock_ == INVALID_SOCKET)
        return false;
#ifdef _WIN32
    if (setsockopt(sock_, level, opt_name, reinterpret_cast<const char*>(&value), sizeof(value)) != 0) {
        note_os_error();
        return false;
    }
#else
    if (setsockopt(sock_, level, opt_name, &value, sizeof(value)) != 0) {
        note_os_error();
        return false;
    }
#endif
    return true;
}

bool TcpSocket::setOptions(const SocketOptions& options) const {
    bool ok = true;
    if (options.no_delay.has_value())
        ok = setNoDelay(*options.no_delay) && ok;
    if (options.keep_alive.has_value())
        ok = setKeepAlive(*options.keep_alive) && ok;
    if (options.send_buffer_size.has_value())
        ok = setSendBufferSize(*options.send_buffer_size) && ok;
    if (options.recv_buffer_size.has_value())
        ok = setRecvBufferSize(*options.recv_buffer_size) && ok;

    if (options.send_timeout_sec.has_value() || options.recv_timeout_sec.has_value()) {
        const unsigned send_sec =
            options.send_timeout_sec.value_or(options.recv_timeout_sec.value_or(0));
        const unsigned recv_sec =
            options.recv_timeout_sec.value_or(options.send_timeout_sec.value_or(0));
        ok = setTimeouts(send_sec, recv_sec) && ok;
    }
    return ok;
}

bool TcpSocket::setNoDelay(const bool enabled) const {
    return set_sock_opt_int(IPPROTO_TCP, TCP_NODELAY, enabled ? 1 : 0);
}

bool TcpSocket::setKeepAlive(const bool enabled) const {
    return set_sock_opt_int(SOL_SOCKET, SO_KEEPALIVE, enabled ? 1 : 0);
}

bool TcpSocket::setSendBufferSize(const std::size_t bytes) const {
    if (bytes == 0 || bytes > static_cast<std::size_t>(std::numeric_limits<int>::max()))
        return false;
    return set_sock_opt_int(SOL_SOCKET, SO_SNDBUF, static_cast<int>(bytes));
}

bool TcpSocket::setRecvBufferSize(const std::size_t bytes) const {
    if (bytes == 0 || bytes > static_cast<std::size_t>(std::numeric_limits<int>::max()))
        return false;
    return set_sock_opt_int(SOL_SOCKET, SO_RCVBUF, static_cast<int>(bytes));
}

bool TcpSocket::setTimeouts(const unsigned send_timeout_sec, const unsigned recv_timeout_sec) const {
    if (sock_ == INVALID_SOCKET)
        return false;
#ifdef _WIN32
    const DWORD send_ms = send_timeout_sec * 1000u;
    const DWORD recv_ms = recv_timeout_sec * 1000u;
    if (setsockopt(sock_, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&send_ms), sizeof(send_ms)) !=
        0) {
        note_os_error();
        return false;
    }
    if (setsockopt(sock_, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&recv_ms), sizeof(recv_ms)) !=
        0) {
        note_os_error();
        return false;
    }
#else
    timeval send_tv{};
    send_tv.tv_sec  = static_cast<time_t>(send_timeout_sec);
    send_tv.tv_usec = 0;
    timeval recv_tv{};
    recv_tv.tv_sec  = static_cast<time_t>(recv_timeout_sec);
    recv_tv.tv_usec = 0;
    if (setsockopt(sock_, SOL_SOCKET, SO_SNDTIMEO, &send_tv, sizeof(send_tv)) != 0) {
        note_os_error();
        return false;
    }
    if (setsockopt(sock_, SOL_SOCKET, SO_RCVTIMEO, &recv_tv, sizeof(recv_tv)) != 0) {
        note_os_error();
        return false;
    }
#endif
    return true;
}

} // namespace net
