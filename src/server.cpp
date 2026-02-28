#include "server.hpp"
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <cmath>
#include <dlfcn.h>

BenchmarkServer::BenchmarkServer(int port, const std::string& plugin_path)
    : port_(port), 
      plugin_path_(plugin_path), 
      zmq_context_(1), 
      zmq_socket_(zmq_context_, zmq::socket_type::rep)
{
    load_plugin();

    std::string address = "tcp://*:" + std::to_string(port_);
    zmq_socket_.bind(address);
    std::cout << "[Server] Listening on " << address << std::endl;
}

BenchmarkServer::~BenchmarkServer() {
    if (plugin_handle_) {
        dlclose(plugin_handle_);
    }
}

void BenchmarkServer::load_plugin() {
    plugin_handle_ = dlopen(plugin_path_.c_str(), RTLD_LAZY);
    if (!plugin_handle_) {
        throw std::runtime_error(std::string("Error loading .so: ") + dlerror());
    }

    get_dimensions_ = reinterpret_cast<GetDimensionsFunc>(dlsym(plugin_handle_, "get_dimensions"));
    check_constraints_ = reinterpret_cast<CheckConstraintsFunc>(dlsym(plugin_handle_, "check_constraints"));
    evaluate_ = reinterpret_cast<EvaluateFunc>(dlsym(plugin_handle_, "evaluate"));

    if (!get_dimensions_ || !check_constraints_ || !evaluate_) {
        throw std::runtime_error("Plugin does not contain required functions!");
    }
    
    std::cout << "[Server] Plugin loaded. Dimensions: " << get_dimensions_() << std::endl;
}

void BenchmarkServer::run() {
    std::cout << "[Server] Waiting for vectors..." << std::endl;
    while (true) {
        process_request();
    }
}

void BenchmarkServer::process_request() {
    zmq::message_t request;
    
    (void)zmq_socket_.recv(request, zmq::recv_flags::none);
    
    const double* vector_data = static_cast<const double*>(request.data());
    size_t num_elements = request.size() / sizeof(double);

    uint8_t status = 0; // 0 = OK, 1 = Wrong dimensions, 2 = Outside domain
    double result = std::numeric_limits<double>::quiet_NaN();

    if (num_elements != get_dimensions_()) {
        status = 1;
        std::cerr << "[Server] Error: Received vector with wrong dimensions!" << std::endl;
    } 
    else if (!check_constraints_(vector_data)) {
        status = 2;
        std::cerr << "[Server] Warning: Vector outside domain!" << std::endl;
    } 
    else {
        result = evaluate_(vector_data);
    }

    std::vector<uint8_t> reply_data(sizeof(uint8_t) + sizeof(double));
    
    reply_data[0] = status;
    std::memcpy(&reply_data[1], &result, sizeof(double));

    zmq::message_t reply(reply_data.begin(), reply_data.end());
    zmq_socket_.send(reply, zmq::send_flags::none);
}