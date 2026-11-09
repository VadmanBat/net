#pragma once
// Created by Vadim 22.06.2026.

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
