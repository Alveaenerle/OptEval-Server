cmake_minimum_required(VERSION 3.14)
project(OptEval-Server CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(PkgConfig REQUIRED)
pkg_check_modules(ZMQ REQUIRED libzmq IMPORTED_TARGET)

include(FetchContent)

set(BUILD_TESTING OFF CACHE INTERNAL "Disable testing completely")
set(CPPZMQ_BUILD_TESTS OFF CACHE INTERNAL "Disable cppzmq tests")

FetchContent_Declare(
    cppzmq
    GIT_REPOSITORY https://github.com/zeromq/cppzmq.git
    GIT_TAG        v4.10.0
)
FetchContent_MakeAvailable(cppzmq)

add_executable(opt_eval_server
    src/main.cpp
    src/server.cpp
    src/server_manager.cpp
    src/plugin_utils.cpp
    src/plugin_registry.cpp
    src/plot_handler.cpp
    src/SessionLogger.cpp
)

target_link_libraries(opt_eval_server PRIVATE cppzmq PkgConfig::ZMQ ${CMAKE_DL_LIBS})

add_custom_command(TARGET opt_eval_server POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_SOURCE_DIR}/plugins
        $<TARGET_FILE_DIR:opt_eval_server>/plugins
    VERBATIM)
