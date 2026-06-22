#include "net/net-initializer.h"
#ifdef _WIN32
#include <winsock2.h>
#endif

namespace net {
NetInitializer NetInitializer::instance;

NetInitializer::NetInitializer() {
#ifdef _WIN32
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) == 0)
        initialized = true;
#else
    initialized = true;
#endif
}

NetInitializer::~NetInitializer() {
    if (initialized) {
#ifdef _WIN32
        WSACleanup();
#endif
        initialized = false;
    }
}

void NetInitializer::ensureInitialized() {
    static_cast<void>(instance);
}
}