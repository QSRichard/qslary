cmake_minimum_required(VERSION 3.2)
project(qslary)

include (cmake/utils.cmake)


set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -g -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined")
# set (CMAKE_CXX_COMPILER "/usr/bin/clang++")

set(LIB_SRC 
    qslary/logger.cc qslary/config.cc 
    qslary/util.cc qslary/thread.cc 
    qslary/currenthread.cc 
    qslary/Condition.cc 
    qslary/Exception.cc qslary/CountDownLatch.cc
    qslary/Fiber.cc
)


# find_library(YAMLCPP yaml-cpp)
# message("*****",${YAMLCPP})

include_directories(.)
include_directories(/home/liushui/Deps/yaml-cpp/include/)
link_directories(/home/liushui/Deps/yaml-cpp/lib/)

add_library(qslary SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(qslary)

# add_library(qslary STATIC ${LIB_SRC})

# add_executable(test tests/test.cc)
#force_redefine_file_macro_for_sources(test)
# add_dependencies(test qslary)
# target_link_libraries(test qslary)

add_executable(test_config tests/test_config.cc)
force_redefine_file_macro_for_sources(test_config)
add_dependencies(test_config qslary)
target_link_libraries(test_config qslary yaml-cpp pthread)



add_executable(test_thread tests/test_thread.cc)
force_redefine_file_macro_for_sources(test_thread)
add_dependencies(test_thread qslary)
target_link_libraries(test_thread qslary yaml-cpp pthread)


add_executable(test_mutex tests/test_mutex.cc)
force_redefine_file_macro_for_sources(test_mutex)
add_dependencies(test_mutex qslary)
target_link_libraries(test_mutex qslary yaml-cpp pthread)

add_executable(test_fiber tests/test_fiber.cc)
force_redefine_file_macro_for_sources(test_fiber)
add_dependencies(test_fiber qslary)
target_link_libraries(test_fiber qslary yaml-cpp pthread)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)