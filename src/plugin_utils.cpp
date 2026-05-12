#include "plugin_utils.hpp"
#include "plugin_registry.hpp"

#include <dlfcn.h>
#include <iostream>
#include <stdexcept>

namespace plugin {

void* loadPlugin(const std::string& pluginId) {
    auto path = Registry::instance().find(pluginId);
    if (!path) {
        throw std::runtime_error("Unknown plugin id: " + pluginId);
    }

    void* handle = ::dlopen(path->c_str(), RTLD_LAZY);
    if (!handle) {
        throw std::runtime_error(std::string("Error loading .so: ") + ::dlerror());
    }

    std::cout << "[PluginLoader] loaded " << *path << std::endl;
    return handle;
}

}
