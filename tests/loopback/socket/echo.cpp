#include "common/fixture.hpp"
#include "test-harness.hpp"

#include <atomic>
#include <net/tcp-server.h>
#include <net/tcp-socket.h>
#include <string>
#include <thread>
#include <vector>

namespace net::test::loopback {
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
    expect(got_listen && server_ok.load(), "server listen ready");

    net::TcpSocket client;
    const bool connected = got_listen && client.connect(k_loopback, k_port);
    expect(connected, "client connect");
    if (connected)
        expect(client.isConnected(), "isConnected true after connect");

    const std::string message = "hello-net-loopback";
    const std::vector<std::uint8_t> payload(message.begin(), message.end());
    if (connected)
        expect(client.sendBytes(payload), "client sendBytes");

    server_thread.join();
    expect(server_ok.load(), "server accept+recv ok");
    expect(received == payload, "echo payload matches");
    expect(peer_ip == k_loopback, "remoteIp is loopback");
    client.disconnect();
}
}
