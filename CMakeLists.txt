cmake_minimum_required(VERSION 3.14)
project(OptEval-Server CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(PkgConfig REQUIRED)
pkg_check_modules(ZMQ REQUIRED libzmq IMPORTED_TARGET)

include(FetchContent)
FetchContent_Declare(
    cppzmq
    GIT_REPOSITORY https://github.com/zeromq/cppzmq.git
    GIT_TAG        v4.10.0
)
FetchContent_MakeAvailable(cppzmq)

add_executable(opt_eval_server
    src/main.cpp
    src/server.cpp    
)

target_link_libraries(opt_eval_server PRIVATE cppzmq PkgConfig::ZMQ)