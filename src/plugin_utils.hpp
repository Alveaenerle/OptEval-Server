#pragma once

#include <string>

namespace plugin {
    // TODO - create more flexible and sophisticated plugin finding logic
    const std::string& getPluginDirectoryPath();
    void* loadPlugin(const std::string &pluginId);
}