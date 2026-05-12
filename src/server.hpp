#pragma once

#include <string>
#include <zmq.hpp>

#include "plugin.hpp"

class BenchmarkServer {
public:
    BenchmarkServer(const std::string& pluginId, const std::string& evalId = "");

    int get_port() const { return port_; }
    void run();

private:
    bool process_request();

    std::string pluginId_;
    std::string evalId_;
    plugin::Plugin plugin_;
    zmq::context_t zmq_context_;
    zmq::socket_t zmq_socket_;
    int port_;
};
