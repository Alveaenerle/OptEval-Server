#pragma once

#include <string>
#include <zmq.hpp>
#include <iostream>
#include <cstring>
#include <cmath>
#include "plugin.hpp"



class BenchmarkServer {
public:
    BenchmarkServer(const std::string& pluginId, const std::string& evalID = "") :
        plugin(pluginId), 
        evalID(evalID),
        pluginId_(pluginId),

        zmq_context_(1),
        zmq_socket_(zmq_context_, zmq::socket_type::rep)
    {
        zmq_socket_.bind("tcp://*:0");
        std::string last_endpoint = zmq_socket_.get(zmq::sockopt::last_endpoint);
        size_t colon_pos = last_endpoint.find_last_of(':');
        port_ = std::stoi(last_endpoint.substr(colon_pos + 1));
        std::cout << "[Server] Listening on " << last_endpoint << std::endl;
    }
    ~BenchmarkServer() = default;
    int get_port() const { return port_; }
    void run();

private:
    int port_;
    plugin::Plugin plugin;    
    zmq::context_t zmq_context_;
    zmq::socket_t zmq_socket_;
    std::string pluginId_;
    std::string evalID;
    int i = 0;
    bool process_request();
};