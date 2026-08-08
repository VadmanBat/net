#include "net/tcp-socket.h"

#include <algorithm>
#include <limits>

namespace net {

bool TcpSocket::sendBytes(const std::uint8_t* data, const std::size_t size) const {
    if (!connected_ || (size > 0 && data == nullptr))
        return false;

    constexpr auto k_max_chunk = static_cast<std::size_t>(std::numeric_limits<int>::max());
    std::size_t total_sent     = 0;
    while (total_sent < size) {
        const std::size_t remaining = size - total_sent;
        const int to_send           = static_cast<int>(std::min(remaining, k_max_chunk));
        const int bytes_sent =
            send(sock_, reinterpret_cast<const char*>(data + total_sent), to_send, 0);
        if (bytes_sent == SOCKET_ERROR) {
            note_os_error();
            return false;
        }
        if (bytes_sent == 0)
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

    const std::size_t chunk =
        std::min(max_size, static_cast<std::size_t>(std::numeric_limits<int>::max()));
    const int bytes_received =
        recv(sock_, reinterpret_cast<char*>(buffer), static_cast<int>(chunk), 0);

    if (bytes_received == SOCKET_ERROR) {
        note_os_error();
        return -1;
    }
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

} // namespace net
