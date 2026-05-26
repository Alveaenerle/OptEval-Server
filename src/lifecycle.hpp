#pragma once

#include <memory>

#include <grpcpp/grpcpp.h>

namespace lifecycle {

// Installs SIGINT/SIGTERM handlers that call Shutdown() on every server
// registered via register_server. Safe to call once at startup.
void install_signal_handlers();

// Registers a running gRPC server so that signal handling can shut it down.
// The lifecycle module holds a shared_ptr, so callers can drop their copy
// after server->Wait() returns.
void register_server(std::shared_ptr<grpc::Server> server);

}
