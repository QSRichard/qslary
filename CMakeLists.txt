cmake_minimum_required(VERSION 3.2)
project(qslary)

include (cmake/utils.cmake)


set(CMAKE_VERBOSE_MAKEFILE ON)
# set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -g -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined")
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -g -std=c++11")
set (CMAKE_CXX_COMPILER "/usr/bin/clang++")

set(LIB_SRC 
    qslary/base/Logger.cc qslary/base/Config.cc 
    qslary/base/util.cc qslary/base/Thread.cc 
    qslary/base/CurrentThread.cc 
    qslary/base/Condition.cc 
    qslary/base/Exception.cc qslary/base/CountDownLatch.cc
    qslary/base/Hook.cc
    qslary/base/Fiber.cc qslary/net/Scheduler.cc
    qslary/net/IOManager.cc 
    qslary/net/Timer.cc
    qslary/net/Timestamp.cc
    qslary/net/Address.cc
    qslary/net/Socket.cc qslary/net/FdManager.cc
    qslary/net/ByteArray.cc qslary/net/TcpServer.cc
    qslary/http/Http.cc qslary/http/HttpParser.cc
    qslary/http/http_request_parser.cc qslary/http/http_response_parser.cc
    # qslary/Timer.cc
)


# find_library(YAMLCPP yaml-cpp)
# message("*****",${YAMLCPP})

include_directories(.)
include_directories(/home/liushui/Deps/yaml-cpp/include/)
link_directories(/home/liushui/Deps/yaml-cpp/lib/)

add_library(qslary SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(qslary)

# add_library(qslary STATIC ${LIB_SRC})


add_executable(test_scheduler tests/test_scheduler.cc)
force_redefine_file_macro_for_sources(test_scheduler)
add_dependencies(test_scheduler qslary)
target_link_libraries(test_scheduler qslary yaml-cpp pthread)


add_executable(test_iomanager tests/test_iomanager.cc)
force_redefine_file_macro_for_sources(test_iomanager)
add_dependencies(test_iomanager qslary)
target_link_libraries(test_iomanager qslary yaml-cpp pthread)


add_executable(test_address tests/test_address.cc)
force_redefine_file_macro_for_sources(test_address)
add_dependencies(test_address qslary)
target_link_libraries(test_address qslary yaml-cpp pthread)

add_executable(test_byteArray tests/test_byteArray.cc)
force_redefine_file_macro_for_sources(test_byteArray)
add_dependencies(test_byteArray qslary)
target_link_libraries(test_byteArray qslary yaml-cpp pthread)

add_executable(test_http_parser tests/test_http_parser.cc)
force_redefine_file_macro_for_sources(test_http_parser)
add_dependencies(test_http_parser qslary)
target_link_libraries(test_http_parser qslary yaml-cpp pthread)

add_executable(test_tcp_server tests/test_tcp_server.cc)
force_redefine_file_macro_for_sources(test_tcp_server)
add_dependencies(test_tcp_server qslary)
target_link_libraries(test_tcp_server qslary yaml-cpp pthread)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)