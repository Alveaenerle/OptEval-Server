#include <iostream>

#include "plugin_registry.hpp"
#include "server_manager.hpp"

int main() {
    std::cout << "[Main] Starting server." << std::endl;

    const auto pluginsDir = plugin::executableDirectory() / "plugins";
    plugin::Registry::instance().scan(pluginsDir);

    ServerManager::getInstance().run();

    std::cout << "[Main] Server closed" << std::endl;
    return 0;
}
