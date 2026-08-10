#include "common/fixture.hpp"
#include "test-harness.hpp"

#include <iostream>

int main() {
    using namespace net::test::loopback;

    std::cout << "net loopback tests (" << k_loopback << ':' << k_port << ")\n";

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
