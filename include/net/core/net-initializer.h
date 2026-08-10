#pragma once

namespace net {
class NetInitializer {
    static NetInitializer instance_;
    bool initialized_ = false;

public:
    NetInitializer();
    ~NetInitializer();

    static void ensureInitialized();
};
}
