cmake_minimum_required(VERSION 3.21)
project(net VERSION 1.0 LANGUAGES CXX)

include(cmake/sources.cmake)

add_library(${PROJECT_NAME} STATIC ${SOURCES})
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)

target_include_directories(${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

if (WIN32)
    target_link_libraries(${PROJECT_NAME} PUBLIC ws2_32)
endif ()

if (PROJECT_IS_TOP_LEVEL)
    include(cmake/top-level.cmake)
endif ()
