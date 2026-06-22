#pragma once

namespace net {
class NetInitializer {
    static NetInitializer instance;
    bool initialized = false;

public:
    NetInitializer();
    ~NetInitializer();

    static void ensureInitialized();
};
}