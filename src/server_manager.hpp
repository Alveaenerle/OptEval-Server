
#pragma once

#include <vector>
#include <string>
#include <thread>
#include <iostream>
#include <zmq.hpp>
#include "server.hpp"


class ServerManager {
public:
    static ServerManager& getInstance() {
        static ServerManager instance;
        return instance;
    }

    ServerManager(const ServerManager&) = delete;
    ServerManager& operator=(const ServerManager&) = delete;

    void run();

private:
    const int port_ = 5000;

    zmq::context_t zmq_context_;
    zmq::socket_t zmq_socket_;
    std::vector<std::thread> threads_;

    ServerManager() : zmq_context_(1), zmq_socket_(zmq_context_, zmq::socket_type::rep) {}

    ~ServerManager() {
        for (auto& t : threads_) {
            if (t.joinable()) {
                t.join();
            }
        }
    }

    void handleRequest(const std::string& problemId);
};