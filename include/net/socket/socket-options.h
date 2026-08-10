#pragma once

#include <cstddef>
#include <optional>

namespace net {
enum class SocketPreset {
    Interactive, ///< Небольшие сообщения, низкая задержка (чат, команды, RPC-подобные операции).
    Bulk,        ///< Большие объемы данных/файлов (большие буферы, более длительные тайм-ауты).
    LongLived,   ///< Соединение может оставаться в режиме ожидания в течение длительного времени (keep-alive).
};

struct SocketOptions {
    std::optional<bool> no_delay;
    std::optional<bool> keep_alive;
    std::optional<std::size_t> send_buffer_size;
    std::optional<std::size_t> recv_buffer_size;
    std::optional<unsigned> send_timeout_sec;
    std::optional<unsigned> recv_timeout_sec;
};

[[nodiscard]] inline SocketOptions make_socket_options(const SocketPreset preset) {
    SocketOptions options;
    switch (preset) {
        case SocketPreset::Interactive:
            options.no_delay         = true;
            options.send_timeout_sec = 10;
            options.recv_timeout_sec = 10;
            break;
        case SocketPreset::Bulk:
            options.no_delay         = true;
            options.send_timeout_sec = 120;
            options.recv_timeout_sec = 120;
            options.send_buffer_size = 1 << 20;
            options.recv_buffer_size = 1 << 20;
            break;
        case SocketPreset::LongLived:
            options.keep_alive       = true;
            options.send_timeout_sec = 60;
            options.recv_timeout_sec = 60;
            break;
    }
    return options;
}
}
