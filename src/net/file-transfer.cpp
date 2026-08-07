#include "net/file-transfer.h"

#include <algorithm>
#include <cstdio>
#include <memory>
#include <vector>

#if defined(__cpp_lib_byteswap) && __cpp_lib_byteswap >= 202110L
#include <bit>
#endif

namespace net {
namespace {
constexpr std::size_t BUFFER_SIZE = 1 << 16; // 64 KiB

struct FileCloser {
    void operator()(FILE* file) const noexcept {
        if (file)
            std::fclose(file);
    }
};

using FilePtr = std::unique_ptr<FILE, FileCloser>;

FilePtr open_file(const fs::path& file_path, const bool write) {
#ifdef _WIN32
    return FilePtr(_wfopen(file_path.wstring().c_str(), write ? L"wb" : L"rb"));
#else
    return FilePtr(std::fopen(file_path.string().c_str(), write ? "wb" : "rb"));
#endif
}

#if defined(__cpp_lib_byteswap) && __cpp_lib_byteswap >= 202110L
std::uint64_t byteswap_u64(const std::uint64_t value) {
    return std::byteswap(value);
}

bool is_little_endian() {
    return std::endian::native == std::endian::little;
}
#else
std::uint64_t byteswap_u64(const std::uint64_t value) {
    return ((value & 0x00000000000000FFULL) << 56) | ((value & 0x000000000000FF00ULL) << 40) |
           ((value & 0x0000000000FF0000ULL) << 24) | ((value & 0x00000000FF000000ULL) << 8) |
           ((value & 0x000000FF00000000ULL) >> 8) | ((value & 0x0000FF0000000000ULL) >> 24) |
           ((value & 0x00FF000000000000ULL) >> 40) | ((value & 0xFF00000000000000ULL) >> 56);
}

bool is_little_endian() {
#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return true;
#elif defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return false;
#elif defined(_WIN32) || defined(__LITTLE_ENDIAN__) || defined(__i386__) || defined(__x86_64__) ||       \
    defined(__amd64__) || defined(_M_IX86) || defined(_M_X64) || defined(_M_ARM) || defined(_M_ARM64) || \
    defined(__aarch64__) || defined(__arm__)
    return true;
#elif defined(__BIG_ENDIAN__)
    return false;
#else
    return true; // assume little-endian
#endif
}
#endif
}

std::uint64_t host_to_network64(const std::uint64_t value) {
    return is_little_endian() ? byteswap_u64(value) : value;
}

std::uint64_t network_to_host64(const std::uint64_t value) {
    return host_to_network64(value);
}

bool send_uint64(const TcpSocket& socket, const std::uint64_t value) {
    const std::uint64_t net = host_to_network64(value);
    return socket.sendBytes(reinterpret_cast<const std::uint8_t*>(&net), sizeof(net));
}

bool receive_uint64(const TcpSocket& socket, std::uint64_t& value) {
    std::uint64_t net_value = 0;
    if (!receive_exact(socket, reinterpret_cast<std::uint8_t*>(&net_value), sizeof(net_value)))
        return false;
    value = network_to_host64(net_value);
    return true;
}

bool receive_exact(const TcpSocket& socket, std::uint8_t* data, const std::size_t size) {
    std::size_t received = 0;
    while (received < size) {
        const Ssize chunk = socket.receiveBytes(data + received, size - received);
        if (chunk <= 0)
            return false;
        received += static_cast<std::size_t>(chunk);
    }
    return true;
}

bool send_file_with_progress(TcpSocket& socket, const fs::path& file_path, const ProgressCallback& callback) {
    if (!fs::exists(file_path) || !fs::is_regular_file(file_path))
        return false;

    const FilePtr file = open_file(file_path, false);
    if (!file)
        return false;

    const std::uint64_t file_size = fs::file_size(file_path);
    if (!send_uint64(socket, file_size)) {
        socket.disconnect();
        return false;
    }

    if (callback)
        callback(0, file_size);

    std::vector<std::uint8_t> buffer(BUFFER_SIZE);
    std::uint64_t sent = 0;

    while (sent < file_size) {
        const std::size_t to_read =
            static_cast<std::size_t>(std::min(file_size - sent, static_cast<std::uint64_t>(BUFFER_SIZE)));
        const std::size_t read = std::fread(buffer.data(), 1, to_read, file.get());
        if (read == 0) {
            socket.disconnect();
            return false;
        }

        if (!socket.sendBytes(buffer.data(), read)) {
            socket.disconnect();
            return false;
        }

        sent += read;
        if (callback)
            callback(sent, file_size);
    }

    return sent == file_size;
}

bool receive_file_with_progress(TcpSocket& socket, const fs::path& file_path, const ProgressCallback& callback) {
    std::uint64_t file_size = 0;
    if (!receive_uint64(socket, file_size))
        return false;

    if (callback)
        callback(0, file_size);

    FilePtr file = open_file(file_path, true);
    if (!file)
        return false;

    std::vector<std::uint8_t> buffer(BUFFER_SIZE);
    std::uint64_t received = 0;

    while (received < file_size) {
        const std::size_t chunk_size =
            static_cast<std::size_t>(std::min(file_size - received, static_cast<std::uint64_t>(BUFFER_SIZE)));

        if (!receive_exact(socket, buffer.data(), chunk_size) ||
            std::fwrite(buffer.data(), 1, chunk_size, file.get()) != chunk_size) {
            file.reset();
            fs::remove(file_path);
            socket.disconnect();
            return false;
        }

        received += chunk_size;
        if (callback)
            callback(received, file_size);
    }

    std::fflush(file.get());
    return true;
}
}
