#pragma once

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <vector>

namespace fs = std::filesystem;

namespace net::test::loopback {
inline constexpr char k_loopback[]    = "127.0.0.1";
inline constexpr std::uint16_t k_port = 49173;
inline constexpr auto k_ready_timeout = std::chrono::seconds(5);

struct Barrier {
    std::mutex mutex;
    std::condition_variable cv;
    bool ready = false;

    void signal() {
        {
            std::lock_guard lock(mutex);
            ready = true;
        }
        cv.notify_all();
    }

    bool wait_for(const std::chrono::milliseconds timeout) {
        std::unique_lock lock(mutex);
        return cv.wait_for(lock, timeout, [this] { return ready; });
    }
};

inline bool write_file(const fs::path& path, const std::vector<std::uint8_t>& data) {
    std::ofstream out(path, std::ios::binary);
    if (!out)
        return false;
    out.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    return static_cast<bool>(out);
}

inline bool read_file(const fs::path& path, std::vector<std::uint8_t>& data) {
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in)
        return false;
    const auto size = in.tellg();
    if (size < 0)
        return false;
    in.seekg(0);
    data.resize(static_cast<std::size_t>(size));
    if (size == 0)
        return true;
    in.read(reinterpret_cast<char*>(data.data()), size);
    return static_cast<bool>(in);
}

void test_connect_invalid_ip();
void test_connect_refused();
void test_echo_and_remote_ip();
void test_receive_exact_and_uint64();
void test_endian_helpers();
void test_socket_options();
void test_is_connected_after_peer_close();
void test_file_transfer();
}
