#pragma once

#include <string>
#include <thread>
#include <vector>
#include <zmq.hpp>

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
    ServerManager()
        : zmq_context_(1),
          zmq_socket_(zmq_context_, zmq::socket_type::rep) {}

    ~ServerManager() {
        for (auto& t : threads_) {
            if (t.joinable()) t.join();
        }
    }

    struct Request {
        std::string head;
        std::string evalId;
        std::string pluginId;
    };

    Request receiveRequest();
    void sendReply(const std::string& payload);
    void handleBenchmark(const std::string& pluginId, const std::string& evalId);
    void handlePlot(const std::string& evalId, const std::string& pluginId);

    static constexpr int kPort = 5000;

    zmq::context_t zmq_context_;
    zmq::socket_t zmq_socket_;
    std::vector<std::thread> threads_;
};
