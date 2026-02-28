#include "server.hpp"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <port> <plugin_path.so>" << std::endl;
        return 1;
    }

    int port = std::stoi(argv[1]);
    std::string plugin_path = argv[2];

    try {
        BenchmarkServer server(port, plugin_path);
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Critical server error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}