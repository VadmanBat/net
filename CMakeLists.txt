cmake_minimum_required(VERSION 3.31)
project(net VERSION 1.0 LANGUAGES CXX)

include(cmake/sources.cmake)

add_library(net STATIC ${net_SOURCES})
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
    include(cmake/top-level.cmake)
endif ()
