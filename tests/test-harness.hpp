#pragma once

#include <iostream>
#include <string_view>

namespace net::test {

inline int g_pass = 0;
inline int g_fail = 0;

inline void expect(const bool cond, const std::string_view label) {
    if (cond) {
        ++g_pass;
        std::cout << "[PASS] " << label << '\n';
    }
    else {
        ++g_fail;
        std::cout << "[FAIL] " << label << '\n';
    }
}

inline void summary(const std::string_view suite) {
    std::cout << "=== " << suite << ": " << g_pass << " passed, " << g_fail << " failed ===\n";
}

inline int failures() {
    return g_fail;
}

} // namespace net::test
