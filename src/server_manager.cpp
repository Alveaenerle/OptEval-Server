#include "server_manager.hpp"

#include <iostream>
#include <memory>

#include "plot_handler.hpp"
#include "server.hpp"

namespace {
const std::string kPlotCommand = "REQUEST_PLOT_DATA";
}

void ServerManager::run() {
    zmq_socket_.bind("tcp://*:" + std::to_string(kPort));
    std::cout << "[ServerManager] Listening on port " << kPort << std::endl;

    while (true) {
        Request req = receiveRequest();
        if (req.head.empty()) continue;

        if (req.head == kPlotCommand) {
            handlePlot(req.evalId, req.pluginId);
        } else {
            handleBenchmark(req.head, req.evalId);
        }
    }
}

ServerManager::Request ServerManager::receiveRequest() {
    Request req;

    zmq::message_t frame;
    if (!zmq_socket_.recv(frame, zmq::recv_flags::none)) return req;
    req.head.assign(static_cast<const char*>(frame.data()), frame.size());

    if (!zmq_socket_.get(zmq::sockopt::rcvmore)) return req;
    if (!zmq_socket_.recv(frame, zmq::recv_flags::none)) return req;
    req.evalId.assign(static_cast<const char*>(frame.data()), frame.size());

    if (!zmq_socket_.get(zmq::sockopt::rcvmore)) return req;
    if (!zmq_socket_.recv(frame, zmq::recv_flags::none)) return req;
    req.pluginId.assign(static_cast<const char*>(frame.data()), frame.size());

    return req;
}

void ServerManager::sendReply(const std::string& payload) {
    zmq::message_t reply(payload.data(), payload.size());
    zmq_socket_.send(reply, zmq::send_flags::none);
}

void ServerManager::handlePlot(const std::string& evalId, const std::string& pluginId) {
    std::cout << "[ServerManager] Processing REQUEST_PLOT_DATA for evalID: " << evalId << std::endl;
    sendReply(buildPlotJson(evalId, pluginId));
}

void ServerManager::handleBenchmark(const std::string& pluginId, const std::string& evalId) {
    std::cout << "[ServerManager] Received request for plugin: " << pluginId
              << " with evalID: " << evalId << std::endl;

    std::shared_ptr<BenchmarkServer> server;
    try {
        server = std::make_shared<BenchmarkServer>(pluginId, evalId);
    } catch (const std::exception& e) {
        std::cerr << "[ServerManager] Failed to create BenchmarkServer: " << e.what() << std::endl;
        sendReply("0");
        return;
    }

    const int assignedPort = server->get_port();
    threads_.emplace_back([server]() {
        try {
            server->run();
        } catch (const std::exception& e) {
            std::cerr << "[ServerManager] BenchmarkServer failed on port "
                      << server->get_port() << ": " << e.what() << std::endl;
        }
    });

    sendReply(std::to_string(assignedPort));
}
