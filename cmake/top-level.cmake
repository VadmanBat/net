add_executable(server examples/server.cpp)
target_link_libraries(server PRIVATE net::net)

add_executable(client examples/client.cpp)
target_link_libraries(client PRIVATE net::net)

enable_testing()

add_executable(net-tests tests/loopback.cpp)
target_link_libraries(net-tests PRIVATE net::net)
target_include_directories(net-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
add_test(NAME net-loopback COMMAND net-tests)
