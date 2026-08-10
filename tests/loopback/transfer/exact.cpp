#include "common/fixture.hpp"
#include "test-harness.hpp"

#include <atomic>
#include <net/file-transfer.h>
#include <net/tcp-server.h>
#include <net/tcp-socket.h>
#include <thread>
#include <vector>

namespace net::test::loopback {
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
        expect(false, "receive_exact server listen");
        return;
    }

    net::TcpSocket client;
    if (!client.connect(k_loopback, k_port)) {
        server_thread.join();
        expect(false, "receive_exact client connect");
        return;
    }

    constexpr std::uint64_t k_value = 0x0123456789ABCDEFULL;
    std::vector<std::uint8_t> blob(100'000);
    for (std::size_t i = 0; i < blob.size(); ++i)
        blob[i] = static_cast<std::uint8_t>(i & 0xFF);

    expect(net::send_uint64(client, k_value), "send_uint64");
    expect(client.sendBytes(blob), "send large blob");

    server_thread.join();
    expect(server_ok.load(), "server received exact payload");
    expect(got_value == k_value, "uint64 roundtrip");
    expect(got_blob == blob, "receive_exact large blob");
}
}
