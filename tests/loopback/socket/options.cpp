#include "common/fixture.hpp"
#include "test-harness.hpp"

#include <atomic>
#include <net/tcp-server.h>
#include <net/tcp-socket.h>
#include <thread>

namespace net::test::loopback {
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
        expect(false, "options server listen");
        return;
    }

    net::TcpSocket client;
    if (!client.connect(k_loopback, k_port)) {
        server_thread.join();
        expect(false, "options client connect");
        return;
    }

    net::SocketOptions client_opts;
    client_opts.no_delay         = true;
    client_opts.keep_alive       = true;
    client_opts.send_buffer_size = 256 * 1024;
    client_opts.recv_buffer_size = 256 * 1024;
    client_opts.send_timeout_sec = 5;
    client_opts.recv_timeout_sec = 5;
    expect(client.setOptions(client_opts), "client setOptions");
    expect(client.noDelay(), "noDelay after setOptions");
    expect(client.keepAlive(), "keepAlive after setOptions");
    expect(client.sendBufferSize() >= 256 * 1024 / 2, "sendBufferSize applied");
    expect(client.recvBufferSize() >= 256 * 1024 / 2, "recvBufferSize applied");

    expect(client.setNoDelay(false), "setNoDelay false");
    expect(!client.noDelay(), "noDelay reads false");

    const std::uint8_t one = 1;
    static_cast<void>(client.sendBytes(&one, 1));
    server_thread.join();

    expect(server_ok.load(), "options accept ok");
    expect(peer_no_delay, "accepted NoDelay from ServerOptions");
}
}
