#pragma once

#include <cstddef>
#include <optional>

namespace net {
struct SocketOptions {
    std::optional<bool> no_delay;
    std::optional<bool> keep_alive;
    std::optional<std::size_t> send_buffer_size;
    std::optional<std::size_t> recv_buffer_size;
    std::optional<unsigned> send_timeout_sec;
    std::optional<unsigned> recv_timeout_sec;
};
}
