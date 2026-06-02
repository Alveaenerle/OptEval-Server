#include "lifecycle.hpp"

#include <atomic>
#include <csignal>
#include <iostream>
#include <mutex>
#include <vector>

namespace lifecycle {

namespace {

std::mutex g_mu;
std::vector<std::shared_ptr<grpc::Server>> g_servers;
std::atomic<bool> g_shutting_down{false};

void handler(int sig) {
    if (g_shutting_down.exchange(true)) return;  // second signal: ignore
    std::cerr << "[lifecycle] received signal " << sig << ", shutting down" << std::endl;
    std::lock_guard<std::mutex> lk(g_mu);
    for (auto& s : g_servers) {
        if (s) s->Shutdown();
    }
}

}

void install_signal_handlers() {
    std::signal(SIGINT, handler);
    std::signal(SIGTERM, handler);
}

void register_server(std::shared_ptr<grpc::Server> server) {
    std::lock_guard<std::mutex> lk(g_mu);
    g_servers.push_back(std::move(server));
}

bool is_shutting_down() {
    return g_shutting_down.load(std::memory_order_acquire);
}

}
