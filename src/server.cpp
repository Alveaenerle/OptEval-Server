#include "server.hpp"
#include <iostream>
#include <cstring>
#include <cmath>

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

    /*
        0 - NAN_VALUE_RETURNED
        1 - BUDGET_EXHAUSTED
        2 - TARGET_REACHED
        3 - EVALUATION_DENIED
        4 - OUT_OF_BOUND_VIOLATION
        5 - DIMENSION_MISMATCH
        6 - PLUGIN_RUNTIME_ERROR
        7 - UNSUPPORTED_PLUGIN_ID
    */
    uint8_t status = 0;
    double result = std::numeric_limits<double>::quiet_NaN();

    if (num_elements != plugin.get().get_dimensions_()) {
        status |= 1 << 4;
        std::cerr << "[Server] Error: Received vector with wrong dimensions!" << std::endl;
    } 
    else if (!plugin.get().check_constraints_(vector_data)) {
        status = 1 << 5;
        std::cerr << "[Server] Warning: Vector outside domain!" << std::endl;
    } 
    else {
        result = plugin.get().evaluate_(vector_data);
    }

    std::vector<uint8_t> reply_data(sizeof(uint8_t) + sizeof(double));
    
    reply_data[0] = status;
    std::memcpy(&reply_data[1], &result, sizeof(double));

    zmq::message_t reply(reply_data.begin(), reply_data.end());
    zmq_socket_.send(reply, zmq::send_flags::none);
}