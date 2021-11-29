cmake_minimum_required(VERSION 3.2)

project(staticDynamic)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -g -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function")

set(LIB_SRC test.cc)

add_library(mytest SHARED ${LIB_SRC})
# add_library(mytest STATIC ${LIB_SRC})

add_executable(main main.cc)

add_dependencies(main mytest)

target_link_libraries(main mytest)