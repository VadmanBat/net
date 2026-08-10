#pragma once

#include "net/socket/socket-options.h"

namespace net {
struct ServerOptions {
    bool reuse_address = true;
    SocketOptions accepted{};
};
}
