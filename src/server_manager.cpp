#include "server_manager.hpp"
#include <memory>

void ServerManager::run() {
    zmq_socket_.bind("tcp://*:" + std::to_string(port_));
    std::cout << "[ServerManager] Listening on port " << port_ << std::endl;

    while (true) {
        zmq::message_t request;
        auto recv_res = zmq_socket_.recv(request, zmq::recv_flags::none);
        if (!recv_res) continue;

        std::string problemId(static_cast<const char*>(request.data()), request.size());
        std::cout << "[ServerManager] Received request for plugin: " << problemId << std::endl;

        handleRequest(problemId);
    }
}


void ServerManager::handleRequest(const std::string& problemId) {
    auto server = std::make_shared<BenchmarkServer>(problemId);
    int assignedPort = server->get_port();
    
    threads_.emplace_back([this, server]() {
        try {
            server->run();
        } catch (const std::exception& e) {
            std::cerr << "[ServerManager] BenchmarkServer failed to start on port " << server->get_port() << ": " << e.what() << std::endl;
        }
    });

    std::string portStr = std::to_string(assignedPort);
    zmq::message_t reply(portStr.data(), portStr.size());
    zmq_socket_.send(reply, zmq::send_flags::none);
}