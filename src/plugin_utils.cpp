#include <stdexcept>
#include <dlfcn.h>
#include <iostream>
#include "plugin_utils.hpp"

const std::string& plugin::getPluginDirectoryPath() {
    static std::string path("/home/dominik/Documents/inzynierka/OptEval-Server/plugins/");
    return path;
}

void* plugin::loadPlugin(const std::string &pluginId) {
    std::string absolutePath(plugin::getPluginDirectoryPath() + pluginId);

    const char* successMessage = "[Server] Plugin loaded.\n";
    void* pluginHandle = dlopen(absolutePath.c_str(), RTLD_LAZY);
    if (!pluginHandle) {
        throw std::runtime_error(std::string("Error loading .so: ") + dlerror());
    }

    std::cout << successMessage;
    return pluginHandle;
}