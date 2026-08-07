#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <net/tcp-socket.h>

namespace fs = std::filesystem;

namespace net {
using ProgressCallback = std::function<void(std::uint64_t sent, std::uint64_t total)>;

std::uint64_t host_to_network64(std::uint64_t value);
std::uint64_t network_to_host64(std::uint64_t value);

[[nodiscard]] bool send_uint64(const TcpSocket& socket, std::uint64_t value);
[[nodiscard]] bool receive_uint64(const TcpSocket& socket, std::uint64_t& value);

[[nodiscard]] bool receive_exact(const TcpSocket& socket, std::uint8_t* data, std::size_t size);

[[nodiscard]] bool send_file_with_progress(TcpSocket& socket, const fs::path& file_path,
                                           const ProgressCallback& callback = {});
[[nodiscard]] bool receive_file_with_progress(TcpSocket& socket, const fs::path& file_path,
                                              const ProgressCallback& callback = {});
}
