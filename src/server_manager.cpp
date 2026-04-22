#include "server_manager.hpp"
#include "SessionLogger.hpp"
#include "plot_handler.hpp"
#include <memory>

void ServerManager::run() {
    zmq_socket_.bind("tcp://*:" + std::to_string(port_));
    std::cout << "[ServerManager] Listening on port " << port_ << std::endl;

    while (true) {
        zmq::message_t request;
        auto recv_res = zmq_socket_.recv(request, zmq::recv_flags::none);
        if (!recv_res) continue;

        std::string problemId(static_cast<const char*>(request.data()), request.size());
        
        std::string evalID = "";
        std::string pluginID = "";
        if (zmq_socket_.get(zmq::sockopt::rcvmore)) {
            zmq::message_t eval_msg;
            auto recv_res2 = zmq_socket_.recv(eval_msg, zmq::recv_flags::none);
            if (recv_res2) {
                evalID = std::string(static_cast<const char*>(eval_msg.data()), eval_msg.size());
                
                if (zmq_socket_.get(zmq::sockopt::rcvmore)) {
                    zmq::message_t plugin_msg;
                    auto recv_res3 = zmq_socket_.recv(plugin_msg, zmq::recv_flags::none);
                    if (recv_res3) {
                        pluginID = std::string(static_cast<const char*>(plugin_msg.data()), plugin_msg.size());
                    }
                }
            }
        }

        if (problemId == "REQUEST_PLOT_DATA") {
            std::cout << "[ServerManager] Processing REQUEST_PLOT_DATA for evalID: " << evalID << std::endl;
            std::string json_reply = buildPlotJson(evalID, pluginID);
            zmq::message_t reply(json_reply.data(), json_reply.size());
            zmq_socket_.send(reply, zmq::send_flags::none);
            continue;
        }

        std::cout << "[ServerManager] Received request for plugin: " << problemId << " with evalID: " << evalID << std::endl;

        handleRequest(problemId, evalID);
    }
}


void ServerManager::handleRequest(const std::string& problemId, const std::string& evalID) {
    auto server = std::make_shared<BenchmarkServer>(problemId, evalID);
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