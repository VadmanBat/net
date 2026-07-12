#include "net/tcp-socket.h"
#include "net/net-initializer.h"

namespace net {
TcpSocket::TcpSocket() {
    NetInitializer::ensureInitialized();
}

TcpSocket::TcpSocket(const SOCKET existing_socket) : sock(existing_socket), connected(true) {
    NetInitializer::ensureInitialized();
}

TcpSocket::~TcpSocket() {
    disconnect();
}

bool TcpSocket::connect(const std::string& ip, const uint16_t port) {
    if (connected)
        disconnect();
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET)
        return false;
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons(port);
    inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr);
    if (::connect(sock, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == SOCKET_ERROR) {
        close_socket(sock);
        sock = INVALID_SOCKET;
        return false;
    }
    connected = true;
    return true;
}

bool TcpSocket::disconnect() {
    if (sock != INVALID_SOCKET) {
        shutdown(sock, SHUT_RDWR);
        close_socket(sock);
        sock = INVALID_SOCKET;
    }
    connected = false;
    return true;
}

bool TcpSocket::sendBytes(const uint8_t* data, const size_t size) const {
    if (!connected)
        return false;
    size_t total_sent = 0;
    while (total_sent < size) {
        const int bytes_sent = send(sock,
                                    reinterpret_cast<const char*>(data + total_sent),
                                    static_cast<int>(size - total_sent),
                                    0);
        if (bytes_sent == SOCKET_ERROR)
            return false;
        total_sent += static_cast<size_t>(bytes_sent);
    }
    return true;
}

bool TcpSocket::sendBytes(const std::vector<uint8_t>& data) const {
    return sendBytes(data.data(), data.size());
}

ssize_t TcpSocket::receiveBytes(uint8_t* buffer, const size_t max_size) const {
    if (!connected || !buffer || max_size == 0)
        return -1;

    const int bytes_received = recv(sock,
                                    reinterpret_cast<char*>(buffer),
                                    static_cast<int>(max_size),
                                    0);

    if (bytes_received == SOCKET_ERROR)
        return -1;
    return static_cast<ssize_t>(bytes_received);
}

std::vector<uint8_t> TcpSocket::receiveBytes(const size_t max_size) const {
    std::vector<uint8_t> buf(max_size);
    const ssize_t received = receiveBytes(buf.data(), max_size);
    if (received <= 0)
        return {};
    buf.resize(static_cast<size_t>(received));
    return buf;
}

bool TcpSocket::isConnected() const {
    return connected;
}

std::string TcpSocket::getRemoteIp() const {
    if (!connected || sock == INVALID_SOCKET)
        return "";
    sockaddr_in addr{};
    socklen_t addr_size = sizeof(addr);
#ifdef _WIN32
    if (getpeername(sock, reinterpret_cast<sockaddr*>(&addr), reinterpret_cast<int*>(&addr_size)) == SOCKET_ERROR)
#else
    if (getpeername(sock, reinterpret_cast<sockaddr*>(&addr), &addr_size) == SOCKET_ERROR)
#endif
        return "";
    char ip_str[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &addr.sin_addr, ip_str, INET_ADDRSTRLEN) == nullptr)
        return "";
    return {ip_str};
}
}