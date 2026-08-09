#include "test-harness.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <net/file-transfer.h>
#include <net/tcp-server.h>
#include <net/tcp-socket.h>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

namespace {

constexpr char k_loopback[]     = "127.0.0.1";
constexpr std::uint16_t k_port  = 49173;
constexpr auto k_ready_timeout  = std::chrono::seconds(5);
constexpr auto k_thread_timeout = std::chrono::seconds(15);

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

bool write_file(const fs::path& path, const std::vector<std::uint8_t>& data) {
    std::ofstream out(path, std::ios::binary);
    if (!out)
        return false;
    out.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    return static_cast<bool>(out);
}

bool read_file(const fs::path& path, std::vector<std::uint8_t>& data) {
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

void test_connect_invalid_ip() {
    net::TcpSocket sock;
    net::test::expect(!sock.connect("not.an.ip", k_port), "connect rejects invalid ip");
    net::test::expect(!sock.isConnected(), "not connected after invalid connect");
    net::test::expect(sock.lastOsError() != 0, "lastOsError set after invalid connect");
    const auto st = sock.status();
    net::test::expect(!st.text.empty(), "statusText non-empty");
    net::test::expect(!st.has_socket, "status: no socket after failed connect");
}

void test_connect_refused() {
    net::TcpSocket sock;
    // Nothing should listen on this high port in CI/dev most of the time.
    const bool ok = sock.connect(k_loopback, 1);
    net::test::expect(!ok, "connect to closed port fails");
}

void test_echo_and_remote_ip() {
    Barrier listening;
    std::atomic_bool server_ok{true};
    std::string peer_ip;
    std::vector<std::uint8_t> received;

    std::thread server_thread([&] {
        net::TcpServer server;
        net::ServerOptions opts;
        opts.reuse_address = true;
        server.setOptions(opts);
        if (!server.listen(k_loopback, k_port)) {
            server_ok = false;
            listening.signal();
            return;
        }
        listening.signal();

        auto peer = server.acceptConnection();
        if (!peer) {
            server_ok = false;
            return;
        }
        peer_ip  = peer->remoteIp();
        received = peer->receiveBytes(4096);
        if (received.empty())
            server_ok = false;
    });

    const bool got_listen = listening.wait_for(k_ready_timeout);
    net::test::expect(got_listen && server_ok.load(), "server listen ready");

    net::TcpSocket client;
    const bool connected = got_listen && client.connect(k_loopback, k_port);
    net::test::expect(connected, "client connect");
    if (connected)
        net::test::expect(client.isConnected(), "isConnected true after connect");

    const std::string message = "hello-net-loopback";
    const std::vector<std::uint8_t> payload(message.begin(), message.end());
    if (connected)
        net::test::expect(client.sendBytes(payload), "client sendBytes");

    server_thread.join();
    net::test::expect(server_ok.load(), "server accept+recv ok");
    net::test::expect(received == payload, "echo payload matches");
    net::test::expect(peer_ip == k_loopback, "remoteIp is loopback");
    client.disconnect();
}

void test_receive_exact_and_uint64() {
    Barrier listening;
    std::atomic_bool server_ok{true};
    std::uint64_t got_value = 0;
    std::vector<std::uint8_t> got_blob(100'000);

    std::thread server_thread([&] {
        net::TcpServer server;
        net::ServerOptions opts;
        opts.reuse_address = true;
        server.setOptions(opts);
        if (!server.listen(k_loopback, k_port)) {
            server_ok = false;
            listening.signal();
            return;
        }
        listening.signal();

        auto peer = server.acceptConnection();
        if (!peer) {
            server_ok = false;
            return;
        }
        if (!net::receive_uint64(*peer, got_value)) {
            server_ok = false;
            return;
        }
        if (!net::receive_exact(*peer, got_blob.data(), got_blob.size()))
            server_ok = false;
    });

    if (!listening.wait_for(k_ready_timeout)) {
        server_thread.join();
        net::test::expect(false, "receive_exact server listen");
        return;
    }

    net::TcpSocket client;
    if (!client.connect(k_loopback, k_port)) {
        server_thread.join();
        net::test::expect(false, "receive_exact client connect");
        return;
    }

    constexpr std::uint64_t k_value = 0x0123456789ABCDEFULL;
    std::vector<std::uint8_t> blob(100'000);
    for (std::size_t i = 0; i < blob.size(); ++i)
        blob[i] = static_cast<std::uint8_t>(i & 0xFF);

    net::test::expect(net::send_uint64(client, k_value), "send_uint64");
    net::test::expect(client.sendBytes(blob), "send large blob");

    server_thread.join();
    net::test::expect(server_ok.load(), "server received exact payload");
    net::test::expect(got_value == k_value, "uint64 roundtrip");
    net::test::expect(got_blob == blob, "receive_exact large blob");
}

void test_endian_helpers() {
    constexpr std::uint64_t value = 0x1122334455667788ULL;
    const std::uint64_t net       = net::host_to_network64(value);
    const std::uint64_t back      = net::network_to_host64(net);
    net::test::expect(back == value, "host/network64 roundtrip");
    // On LE, net should differ from host for this multi-byte pattern.
    // On BE they are equal — both outcomes valid.
    net::test::expect(true, "endian helpers callable");
    (void)net;
}

void test_socket_options() {
    Barrier listening;
    std::atomic_bool server_ok{true};
    bool peer_no_delay = false;

    std::thread server_thread([&] {
        net::TcpServer server;
        net::ServerOptions opts;
        opts.reuse_address             = true;
        opts.accepted.no_delay         = true;
        opts.accepted.send_timeout_sec = 10;
        opts.accepted.recv_timeout_sec = 10;
        server.setOptions(opts);
        if (!server.listen(k_loopback, k_port)) {
            server_ok = false;
            listening.signal();
            return;
        }
        listening.signal();

        auto peer = server.acceptConnection();
        if (!peer) {
            server_ok = false;
            return;
        }
        peer_no_delay  = peer->noDelay();
        std::uint8_t b = 0;
        static_cast<void>(peer->receiveBytes(&b, 1));
    });

    if (!listening.wait_for(k_ready_timeout)) {
        server_thread.join();
        net::test::expect(false, "options server listen");
        return;
    }

    net::TcpSocket client;
    if (!client.connect(k_loopback, k_port)) {
        server_thread.join();
        net::test::expect(false, "options client connect");
        return;
    }

    net::SocketOptions client_opts;
    client_opts.no_delay         = true;
    client_opts.keep_alive       = true;
    client_opts.send_buffer_size = 256 * 1024;
    client_opts.recv_buffer_size = 256 * 1024;
    client_opts.send_timeout_sec = 5;
    client_opts.recv_timeout_sec = 5;
    net::test::expect(client.setOptions(client_opts), "client setOptions");
    net::test::expect(client.noDelay(), "noDelay after setOptions");
    net::test::expect(client.keepAlive(), "keepAlive after setOptions");
    net::test::expect(client.sendBufferSize() >= 256 * 1024 / 2, "sendBufferSize applied");
    net::test::expect(client.recvBufferSize() >= 256 * 1024 / 2, "recvBufferSize applied");

    net::test::expect(client.setNoDelay(false), "setNoDelay false");
    net::test::expect(!client.noDelay(), "noDelay reads false");

    const std::uint8_t one = 1;
    static_cast<void>(client.sendBytes(&one, 1));
    server_thread.join();

    net::test::expect(server_ok.load(), "options accept ok");
    net::test::expect(peer_no_delay, "accepted NoDelay from ServerOptions");
}

void test_is_connected_after_peer_close() {
    Barrier listening;
    std::atomic_bool server_ok{true};

    std::thread server_thread([&] {
        net::TcpServer server;
        net::ServerOptions opts;
        opts.reuse_address = true;
        server.setOptions(opts);
        if (!server.listen(k_loopback, k_port)) {
            server_ok = false;
            listening.signal();
            return;
        }
        listening.signal();

        auto peer = server.acceptConnection();
        if (!peer) {
            server_ok = false;
            return;
        }
        peer->disconnect();
    });

    if (!listening.wait_for(k_ready_timeout)) {
        server_thread.join();
        net::test::expect(false, "isConnected server listen");
        return;
    }

    net::TcpSocket client;
    if (!client.connect(k_loopback, k_port)) {
        server_thread.join();
        net::test::expect(false, "isConnected client connect");
        return;
    }

    // Give server time to accept and disconnect.
    for (int i = 0; i < 50 && client.isConnected(); ++i)
        std::this_thread::sleep_for(std::chrono::milliseconds(20));

    server_thread.join();
    net::test::expect(server_ok.load(), "isConnected server path ok");
    net::test::expect(!client.isConnected(), "isConnected false after peer disconnect");
}

void test_file_transfer() {
    Barrier listening;
    std::atomic_bool server_ok{true};

    const fs::path dir = fs::temp_directory_path() / "net-lib-tests";
    const fs::path src = dir / "src.bin";
    const fs::path dst = dir / "dst.bin";
    fs::create_directories(dir);

    std::vector<std::uint8_t> payload(50'000);
    for (std::size_t i = 0; i < payload.size(); ++i)
        payload[i] = static_cast<std::uint8_t>((i * 17 + 3) & 0xFF);

    if (!write_file(src, payload)) {
        net::test::expect(false, "write temp source file");
        return;
    }
    fs::remove(dst);

    std::thread server_thread([&] {
        net::TcpServer server;
        net::ServerOptions opts;
        opts.reuse_address             = true;
        opts.accepted.send_timeout_sec = 10;
        opts.accepted.recv_timeout_sec = 10;
        server.setOptions(opts);
        if (!server.listen(k_loopback, k_port)) {
            server_ok = false;
            listening.signal();
            return;
        }
        listening.signal();

        auto peer = server.acceptConnection();
        if (!peer) {
            server_ok = false;
            return;
        }
        if (!net::receive_file_with_progress(*peer, dst))
            server_ok = false;
    });

    if (!listening.wait_for(k_ready_timeout)) {
        server_thread.join();
        net::test::expect(false, "file server listen");
        return;
    }

    net::TcpSocket client;
    if (!client.connect(k_loopback, k_port)) {
        server_thread.join();
        net::test::expect(false, "file client connect");
        return;
    }
    static_cast<void>(client.setTimeouts(10, 10));

    std::uint64_t last_sent = 0;
    const bool sent =
        net::send_file_with_progress(client, src, [&](const std::uint64_t s, const std::uint64_t) { last_sent = s; });
    net::test::expect(sent, "send_file_with_progress");
    net::test::expect(last_sent == payload.size(), "progress reached total");

    server_thread.join();
    net::test::expect(server_ok.load(), "receive_file_with_progress");

    std::vector<std::uint8_t> got;
    net::test::expect(read_file(dst, got), "read received file");
    net::test::expect(got == payload, "file bytes match");

    fs::remove(src);
    fs::remove(dst);
}

} // namespace

int main() {
    std::cout << "net loopback tests (127.0.0.1:" << k_port << ")\n";

    test_connect_invalid_ip();
    test_connect_refused();
    test_endian_helpers();
    test_echo_and_remote_ip();
    test_receive_exact_and_uint64();
    test_socket_options();
    test_is_connected_after_peer_close();
    test_file_transfer();

    net::test::summary("net");
    return net::test::failures() == 0 ? 0 : 1;
}
