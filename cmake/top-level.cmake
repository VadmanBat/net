add_executable(server examples/server.cpp)
target_link_libraries(server PRIVATE ${PROJECT_NAME}::${PROJECT_NAME})

add_executable(client examples/client.cpp)
target_link_libraries(client PRIVATE ${PROJECT_NAME}::${PROJECT_NAME})

enable_testing()

add_executable(net-tests
        tests/loopback/main.cpp
        tests/loopback/connect/connect.cpp
        tests/loopback/socket/echo.cpp
        tests/loopback/socket/options.cpp
        tests/loopback/socket/is-connected.cpp
        tests/loopback/transfer/exact.cpp
        tests/loopback/transfer/endian.cpp
        tests/loopback/transfer/file-transfer.cpp
)
target_link_libraries(net-tests PRIVATE ${PROJECT_NAME}::${PROJECT_NAME})
target_include_directories(net-tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/loopback
)
add_test(NAME net-loopback COMMAND net-tests)
