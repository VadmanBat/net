#include "net/tcp-socket.h"

#include <sstream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#else
#include <cerrno>
#include <cstring>
#endif

namespace net {
namespace {
std::string os_error_text(const int code) {
    if (code == 0)
        return {};
#ifdef _WIN32
    char* buffer = nullptr;
    const DWORD n =
        FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                       nullptr, static_cast<DWORD>(code), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       reinterpret_cast<LPSTR>(&buffer), 0, nullptr);
    if (n == 0 || buffer == nullptr)
        return "WSA/OS error " + std::to_string(code);
    std::string text(buffer, n);
    LocalFree(buffer);
    while (!text.empty() && (text.back() == '\n' || text.back() == '\r' || text.back() == ' '))
        text.pop_back();
    return text;
#else
    const char* msg = std::strerror(code);
    return msg ? std::string(msg) : ("errno " + std::to_string(code));
#endif
}

bool read_endpoint(const SOCKET sock, const bool peer, std::string& ip, std::uint16_t& port) {
    sockaddr_in addr{};
#ifdef _WIN32
    int len = sizeof(addr);
#else
    socklen_t len = sizeof(addr);
#endif
    const int rc = peer ? getpeername(sock, reinterpret_cast<sockaddr*>(&addr), &len)
                        : getsockname(sock, reinterpret_cast<sockaddr*>(&addr), &len);
    if (rc == SOCKET_ERROR)
        return false;

    char ip_str[INET_ADDRSTRLEN]{};
    if (inet_ntop(AF_INET, &addr.sin_addr, ip_str, INET_ADDRSTRLEN) == nullptr)
        return false;
    ip   = ip_str;
    port = ntohs(addr.sin_port);
    return true;
}
} // namespace

void TcpSocket::note_os_error() const {
#ifdef _WIN32
    last_os_error_ = WSAGetLastError();
#else
    last_os_error_ = errno;
#endif
}

void TcpSocket::clear_os_error() const {
    last_os_error_ = 0;
}

int TcpSocket::lastOsError() const noexcept {
    return last_os_error_;
}

SocketStatus TcpSocket::status() const {
    SocketStatus s;
    s.has_socket         = sock_ != INVALID_SOCKET;
    s.flag_connected     = connected_;
    s.last_os_error      = last_os_error_;
    s.last_os_error_text = os_error_text(last_os_error_);

    if (s.has_socket) {
        s.live_connected = isConnected();
        static_cast<void>(read_endpoint(sock_, false, s.local_ip, s.local_port));
        static_cast<void>(read_endpoint(sock_, true, s.remote_ip, s.remote_port));
        s.no_delay         = noDelay();
        s.keep_alive       = keepAlive();
        s.send_buffer_size = sendBufferSize();
        s.recv_buffer_size = recvBufferSize();
    }

    std::ostringstream out;
    out << "TcpSocket status\n"
        << "  has_socket:     " << (s.has_socket ? "yes" : "no") << '\n'
        << "  flag_connected: " << (s.flag_connected ? "yes" : "no") << '\n'
        << "  live_connected: " << (s.live_connected ? "yes" : "no") << '\n'
        << "  local:          " << (s.local_ip.empty() ? "-" : s.local_ip) << ':' << s.local_port << '\n'
        << "  remote:         " << (s.remote_ip.empty() ? "-" : s.remote_ip) << ':' << s.remote_port << '\n'
        << "  no_delay:       " << (s.no_delay ? "yes" : "no") << '\n'
        << "  keep_alive:     " << (s.keep_alive ? "yes" : "no") << '\n'
        << "  send_buf:       " << s.send_buffer_size << '\n'
        << "  recv_buf:       " << s.recv_buffer_size << '\n'
        << "  last_os_error:  " << s.last_os_error;
    if (!s.last_os_error_text.empty())
        out << " (" << s.last_os_error_text << ')';
    out << '\n';
    s.text = out.str();
    return s;
}

std::string TcpSocket::statusText() const {
    return status().text;
}
} // namespace net
