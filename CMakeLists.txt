cmake_minimum_required(VERSION 3.16)
project(OptEval-Server CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(FetchContent)

# --- gRPC (brings in protobuf, abseil, c-ares, re2, zlib) --------------------
set(ABSL_PROPAGATE_CXX_STD ON CACHE INTERNAL "")
set(BUILD_TESTING OFF CACHE INTERNAL "")
set(gRPC_BUILD_TESTS OFF CACHE INTERNAL "")
# Disable installs across the dependency tree; we only consume targets in-tree
# and avoiding installs sidesteps export-set issues with abseil/protobuf/utf8_range.
set(protobuf_INSTALL OFF CACHE INTERNAL "")
set(utf8_range_ENABLE_INSTALL OFF CACHE INTERNAL "")
set(ABSL_ENABLE_INSTALL OFF CACHE INTERNAL "")
set(RE2_BUILD_TESTING OFF CACHE INTERNAL "")
set(gRPC_BUILD_CSHARP_EXT OFF CACHE INTERNAL "")
set(gRPC_BUILD_CODEGEN ON CACHE INTERNAL "")
set(gRPC_INSTALL OFF CACHE INTERNAL "")
set(gRPC_BUILD_GRPC_CSHARP_PLUGIN OFF CACHE INTERNAL "")
set(gRPC_BUILD_GRPC_NODE_PLUGIN OFF CACHE INTERNAL "")
set(gRPC_BUILD_GRPC_OBJECTIVE_C_PLUGIN OFF CACHE INTERNAL "")
set(gRPC_BUILD_GRPC_PHP_PLUGIN OFF CACHE INTERNAL "")
set(gRPC_BUILD_GRPC_PYTHON_PLUGIN OFF CACHE INTERNAL "")
set(gRPC_BUILD_GRPC_RUBY_PLUGIN OFF CACHE INTERNAL "")

FetchContent_Declare(
    grpc
    GIT_REPOSITORY https://github.com/grpc/grpc.git
    GIT_TAG        v1.66.1
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(grpc)

# --- nlohmann/json (header-only) ---------------------------------------------
set(JSON_Install OFF CACHE INTERNAL "")
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

# --- Proto codegen -----------------------------------------------------------
set(_PROTO_DIR ${CMAKE_SOURCE_DIR}/src/proto)
set(_PROTO_GEN ${CMAKE_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${_PROTO_GEN})

set(_PROTOS evaluator archivist)
set(_GENERATED_SRCS "")
foreach(p ${_PROTOS})
    set(_proto_file ${_PROTO_DIR}/${p}.proto)
    set(_pb_cc    ${_PROTO_GEN}/${p}.pb.cc)
    set(_pb_h     ${_PROTO_GEN}/${p}.pb.h)
    set(_grpc_cc  ${_PROTO_GEN}/${p}.grpc.pb.cc)
    set(_grpc_h   ${_PROTO_GEN}/${p}.grpc.pb.h)

    add_custom_command(
        OUTPUT  ${_pb_cc} ${_pb_h} ${_grpc_cc} ${_grpc_h}
        COMMAND $<TARGET_FILE:protoc>
                --cpp_out=${_PROTO_GEN}
                --grpc_out=${_PROTO_GEN}
                --plugin=protoc-gen-grpc=$<TARGET_FILE:grpc_cpp_plugin>
                -I ${_PROTO_DIR}
                ${_proto_file}
        DEPENDS ${_proto_file} protoc grpc_cpp_plugin
        COMMENT "Generating protobuf+grpc sources for ${p}.proto"
        VERBATIM
    )
    list(APPEND _GENERATED_SRCS ${_pb_cc} ${_grpc_cc})
endforeach()

add_library(opt_eval_proto STATIC ${_GENERATED_SRCS})
target_include_directories(opt_eval_proto PUBLIC ${_PROTO_GEN})
target_link_libraries(opt_eval_proto PUBLIC grpc++ libprotobuf)

# --- Server executable -------------------------------------------------------
add_executable(opt_eval_server
    src/main.cpp
    src/config.cpp
    src/lifecycle.cpp
    src/evaluator/run.cpp
    src/evaluator/evaluator_service.cpp
    src/evaluator/shm_evaluator.cpp
    src/evaluator/archivist_logger.cpp
    src/evaluator/plugin_utils.cpp
    src/evaluator/plugin_registry.cpp
    src/archivist/run.cpp
    src/archivist/archivist_service.cpp
    src/archivist/plot_handler.cpp
    src/archivist/SessionLogger.cpp
)
target_include_directories(opt_eval_server PRIVATE src)
target_link_libraries(opt_eval_server PRIVATE
    opt_eval_proto
    grpc++
    nlohmann_json::nlohmann_json
    ${CMAKE_DL_LIBS})

add_custom_command(TARGET opt_eval_server POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_SOURCE_DIR}/plugins
        $<TARGET_FILE_DIR:opt_eval_server>/plugins
    VERBATIM)
