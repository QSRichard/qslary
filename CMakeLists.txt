cmake_minimum_required(VERSION 3.2)
project(qslary)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -g -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function")
# set (CMAKE_CXX_COMPILER "/usr/bin/clang")

set(LIB_SRC qslary/logger.cc qslary/config.cc)

# include_directories(/apps/qslary/include)
# link_directories(/apps/qslary/lib)

# find_library(YAMLCPP yaml-cpp)
# message("*****",${YAMLCPP})

include_directories(.)
include_directories(/apps/qslary/usr/local/include/)
link_directories(/apps/qslary/usr/local/lib)

add_library(qslary SHARED ${LIB_SRC})
# add_library(qslary STATIC ${LIB_SRC})

# add_executable(test tests/test.cc)
# add_dependencies(test qslary)
# target_link_libraries(test qslary)

add_executable(test_config tests/test_config.cc)
add_dependencies(test_config qslary)
target_link_libraries(test_config qslary yaml-cpp)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)