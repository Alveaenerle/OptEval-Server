#include <iostream>
#include "server_manager.hpp"

int main(int argc, char* argv[]) {
    std::cout << "[Main] Starting server.\n";
    ServerManager::getInstance().run();
    std::cout << "[Main] Server closed\n";
    return 0;
}