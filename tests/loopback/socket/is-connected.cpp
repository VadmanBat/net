#include "common/fixture.hpp"
#include "test-harness.hpp"

#include <atomic>
#include <chrono>
#include <net/tcp-server.h>
#include <net/tcp-socket.h>
#include <thread>

namespace net::test::loopback {
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

        const auto peer = server.acceptConnection();
        if (!peer) {
            server_ok = false;
            return;
        }
        peer->disconnect();
    });

    if (!listening.wait_for(k_ready_timeout)) {
        server_thread.join();
        expect(false, "isConnected server listen");
        return;
    }

    net::TcpSocket client;
    if (!client.connect(k_loopback, k_port)) {
        server_thread.join();
        expect(false, "isConnected client connect");
        return;
    }

    for (int i = 0; i < 50 && client.isConnected(); ++i)
        std::this_thread::sleep_for(std::chrono::milliseconds(20));

    server_thread.join();
    expect(server_ok.load(), "isConnected server path ok");
    expect(!client.isConnected(), "isConnected false after peer disconnect");
}
}
