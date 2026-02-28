#pragma once

#include <string>
#include <vector>
#include <zmq.hpp>

typedef int (*GetDimensionsFunc)();
typedef bool (*CheckConstraintsFunc)(const double*);
typedef double (*EvaluateFunc)(const double*);

class BenchmarkServer {
public:
    BenchmarkServer(int port, const std::string& plugin_path);
    ~BenchmarkServer();
    void run();

private:
    int port_;
    std::string plugin_path_;
    
    zmq::context_t zmq_context_;
    zmq::socket_t zmq_socket_;

    void* plugin_handle_;

    GetDimensionsFunc get_dimensions_;
    CheckConstraintsFunc check_constraints_;
    EvaluateFunc evaluate_;

    void load_plugin();
    void process_request();
};