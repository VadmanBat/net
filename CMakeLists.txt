cmake_minimum_required(VERSION 3.31)
project(net VERSION 1.0 LANGUAGES CXX)

add_library(net STATIC
        src/net/net-initializer.cpp
        src/net/tcp-socket/tcp-socket.cpp
        src/net/tcp-socket/setters.cpp
        src/net/tcp-socket/getters.cpp
        src/net/tcp-socket/io.cpp
        src/net/tcp-socket/status.cpp
        src/net/tcp-server/tcp-server.cpp
        src/net/file-transfer.cpp
)
add_library(net::net ALIAS net)

target_compile_features(net PUBLIC cxx_std_17)

target_include_directories(net PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

if (WIN32)
    target_link_libraries(net PUBLIC ws2_32)
endif ()

if (PROJECT_IS_TOP_LEVEL)
    add_executable(server examples/server.cpp)
    target_link_libraries(server PRIVATE net::net)

    add_executable(client examples/client.cpp)
    target_link_libraries(client PRIVATE net::net)

    enable_testing()
    add_executable(net-tests tests/loopback.cpp)
    target_link_libraries(net-tests PRIVATE net::net)
    target_include_directories(net-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
    add_test(NAME net-loopback COMMAND net-tests)
endif ()
