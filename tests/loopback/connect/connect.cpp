#include "common/fixture.hpp"
#include "test-harness.hpp"

#include <net/tcp-socket.h>

namespace net::test::loopback {
void test_connect_invalid_ip() {
    net::TcpSocket sock;
    expect(!sock.connect("not.an.ip", k_port), "connect rejects invalid ip");
    expect(!sock.isConnected(), "not connected after invalid connect");
    expect(sock.lastOsError() != 0, "lastOsError set after invalid connect");
    const auto st = sock.status();
    expect(!st.text.empty(), "statusText non-empty");
    expect(!st.has_socket, "status: no socket after failed connect");
}

void test_connect_refused() {
    net::TcpSocket sock;
    const bool ok = sock.connect(k_loopback, 1);
    expect(!ok, "connect to closed port fails");
}
}
