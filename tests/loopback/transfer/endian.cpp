#include "common/fixture.hpp"
#include "test-harness.hpp"

#include <net/file-transfer.h>

namespace net::test::loopback {
void test_endian_helpers() {
    constexpr std::uint64_t value = 0x1122334455667788ULL;
    const std::uint64_t net_v     = net::host_to_network64(value);
    const std::uint64_t back      = net::network_to_host64(net_v);
    expect(back == value, "host/network64 roundtrip");
    expect(true, "endian helpers callable");
    (void)net_v;
}
}
