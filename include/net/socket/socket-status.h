#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace net {
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
}
