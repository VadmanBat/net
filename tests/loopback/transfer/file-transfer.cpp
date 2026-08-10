#include "common/fixture.hpp"
#include "test-harness.hpp"

#include <atomic>
#include <net/file-transfer.h>
#include <net/tcp-server.h>
#include <net/tcp-socket.h>
#include <thread>
#include <vector>

namespace net::test::loopback {
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
        expect(false, "write temp source file");
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
        expect(false, "file server listen");
        return;
    }

    net::TcpSocket client;
    if (!client.connect(k_loopback, k_port)) {
        server_thread.join();
        expect(false, "file client connect");
        return;
    }
    static_cast<void>(client.setTimeouts(10, 10));

    std::uint64_t last_sent = 0;
    const bool sent =
        net::send_file_with_progress(client, src, [&](const std::uint64_t s, const std::uint64_t) { last_sent = s; });
    expect(sent, "send_file_with_progress");
    expect(last_sent == payload.size(), "progress reached total");

    server_thread.join();
    expect(server_ok.load(), "receive_file_with_progress");

    std::vector<std::uint8_t> got;
    expect(read_file(dst, got), "read received file");
    expect(got == payload, "file bytes match");

    fs::remove(src);
    fs::remove(dst);
}
}
