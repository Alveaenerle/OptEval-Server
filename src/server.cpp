#include "server.hpp"

#include <cstring>
#include <iostream>
#include <vector>

#include "SessionLogger.hpp"
#include "evaluator.hpp"

namespace {
constexpr int kReceiveTimeoutMs = 20000;
}

BenchmarkServer::BenchmarkServer(const std::string& pluginId, const std::string& evalId)
    : pluginId_(pluginId),
      evalId_(evalId),
      plugin_(pluginId),
      zmq_context_(1),
      zmq_socket_(zmq_context_, zmq::socket_type::rep) {
    zmq_socket_.bind("tcp://*:0");
    const std::string endpoint = zmq_socket_.get(zmq::sockopt::last_endpoint);
    port_ = std::stoi(endpoint.substr(endpoint.find_last_of(':') + 1));
    std::cout << "[Server] Listening on " << endpoint << std::endl;
}

void BenchmarkServer::run() {
    std::cout << "[Server] Waiting for data..." << std::endl;
    zmq_socket_.set(zmq::sockopt::rcvtimeo, kReceiveTimeoutMs);
    while (process_request()) {
    }
    std::cout << "[Server] Timeout reached. Freeing slot on port " << port_ << std::endl;
}

bool BenchmarkServer::process_request() {
    zmq::message_t request;
    const auto recv_res = zmq_socket_.recv(request, zmq::recv_flags::none);
    if (!recv_res) {
        return false;
    }

    const auto* vector_data = static_cast<const double*>(request.data());
    const std::size_t num_elements = request.size() / sizeof(double);

    const eval::Result result = eval::evaluate(plugin_, vector_data, num_elements);

    std::vector<uint8_t> reply_data(sizeof(uint8_t) + sizeof(double));
    reply_data[0] = result.status;
    std::memcpy(&reply_data[1], &result.value, sizeof(double));

    if (!evalId_.empty()) {
        global_logger.log_evaluation(evalId_, pluginId_, result.value);
    }

    zmq::message_t reply(reply_data.begin(), reply_data.end());
    zmq_socket_.send(reply, zmq::send_flags::none);
    return true;
}
