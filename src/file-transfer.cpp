#include "net/file-transfer.h"
#include "net/tcp-socket.h"

#include <bit>
#include <cstdio>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

namespace net {
constexpr size_t BUFFER_SIZE = 1 << 16; // 64 КиБ — оптимально для сети

constexpr uint64_t host_to_network64(const uint64_t value) {
    if constexpr (std::endian::native == std::endian::little)
        return std::byteswap(value);
    return value;
}

constexpr uint64_t network_to_host64(const uint64_t value) {
    return host_to_network64(value);
}

bool send_uint64(const TcpSocket& socket, const uint64_t value) {
    const uint64_t net = host_to_network64(value);
    return socket.sendBytes(reinterpret_cast<const uint8_t*>(&net), sizeof(net));
}

bool receive_uint64(const TcpSocket& socket, uint64_t& value) {
    uint64_t net_value = 0;
    if (!receive_exact(socket, reinterpret_cast<uint8_t*>(&net_value), sizeof(net_value)))
        return false;
    value = network_to_host64(net_value);
    return true;
}

bool receive_exact(const TcpSocket& socket, uint8_t* data, const size_t size) {
    size_t received = 0;
    while (received < size) {
        const ssize_t chunk = socket.receiveBytes(data + received, size - received);
        if (chunk <= 0)
            return false;
        received += static_cast<size_t>(chunk);
    }
    return true;
}

bool send_file_with_progress(const TcpSocket& socket,
                             const fs::path& file_path,
                             const ProgressCallback& callback) {
    if (!fs::exists(file_path) || !fs::is_regular_file(file_path))
        return false;

    const uint64_t file_size = fs::file_size(file_path);

    if (!send_uint64(socket, file_size))
        return false;

    if (callback)
        callback(0, file_size);

    FILE* file = nullptr;
#ifdef _WIN32
    file = _wfopen(file_path.wstring().c_str(), L"rb");
#else
    file = fopen(file_path.u8string().c_str(), "rb");
#endif

    if (!file)
        return false;

    std::vector<uint8_t> buffer(BUFFER_SIZE);
    uint64_t sent = 0;

    while (sent < file_size) {
        const size_t to_read = std::min(file_size - sent, static_cast<uint64_t>(BUFFER_SIZE));
        const size_t read    = fread(buffer.data(), 1, to_read, file);
        if (read == 0)
            break;

        if (!socket.sendBytes(buffer.data(), read)) {
            fclose(file);
            return false;
        }

        sent += read;
        if (callback)
            callback(sent, file_size);
    }

    fclose(file);
    return sent == file_size;
}

bool receive_file_with_progress(const TcpSocket& socket,
                                const fs::path& file_path,
                                const ProgressCallback& callback) {
    uint64_t file_size;
    if (!receive_uint64(socket, file_size))
        return false;

    if (callback)
        callback(0, file_size);

    FILE* file = nullptr;
#ifdef _WIN32
    file = _wfopen(file_path.wstring().c_str(), L"wb");
#else
    file = fopen(file_path.u8string().c_str(), "wb");
#endif

    if (!file)
        return false;

    std::vector<uint8_t> buffer(BUFFER_SIZE);
    uint64_t received = 0;

    while (received < file_size) {
        const size_t chunk_size = std::min(file_size - received, static_cast<uint64_t>(BUFFER_SIZE));

        if (!receive_exact(socket, buffer.data(), chunk_size) ||
            fwrite(buffer.data(), 1, chunk_size, file) != chunk_size) {
            fclose(file);
            fs::remove(file_path);
            return false;
        }

        received += chunk_size;
        if (callback)
            callback(received, file_size);
    }

    fflush(file);
    fclose(file);
    return true;
}
}