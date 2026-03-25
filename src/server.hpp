#pragma once

#include <string>
#include <vector>
#include <zmq.hpp>
#include "plugin.hpp"



class BenchmarkServer {
public:
    BenchmarkServer(int port, const std::string& pluginPath) :
        port_(port), 
        plugin(pluginPath), 
        zmq_context_(1), 
        zmq_socket_(zmq_context_, zmq::socket_type::rep)
    {
        std::string address = "tcp://*:" + std::to_string(port_);
        zmq_socket_.bind(address);
        std::cout << "[Server] Listening on " << address << std::endl;
    }
    ~BenchmarkServer() {};
    void run();

private:
    int port_;
    Plugin plugin;    
    zmq::context_t zmq_context_;
    zmq::socket_t zmq_socket_;
    void process_request();
};