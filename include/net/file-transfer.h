#pragma once

#include <net/tcp-socket.h>

#include <filesystem>
#include <functional>

namespace fs = std::filesystem;

namespace net {
using ProgressCallback = std::function<void(uint64_t sent, uint64_t total)>;

uint64_t host_to_network64(uint64_t value);
uint64_t network_to_host64(uint64_t value);

bool send_uint64(const TcpSocket& socket, uint64_t value);
bool receive_uint64(const TcpSocket& socket, uint64_t& value);

bool receive_exact(const TcpSocket& socket, uint8_t* data, size_t size);

bool send_file_with_progress(const TcpSocket& socket,
                             const fs::path& file_path,
                             const ProgressCallback& callback);

bool receive_file_with_progress(const TcpSocket& socket,
                                const fs::path& file_path,
                                const ProgressCallback& callback);
}