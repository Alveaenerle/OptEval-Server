#include "plugin_registry.hpp"

#include <climits>
#include <iostream>
#include <unistd.h>

namespace plugin {

Registry& Registry::instance() {
    static Registry r;
    return r;
}

void Registry::scan(const std::filesystem::path& pluginsDir) {
    entries_.clear();

    std::error_code ec;
    if (!std::filesystem::is_directory(pluginsDir, ec)) {
        std::cerr << "[PluginRegistry] plugins directory not found: " << pluginsDir << std::endl;
        return;
    }

    for (auto it = std::filesystem::recursive_directory_iterator(
                pluginsDir, std::filesystem::directory_options::skip_permission_denied, ec);
         it != std::filesystem::recursive_directory_iterator();
         it.increment(ec)) {
        if (ec) break;
        if (!it->is_regular_file(ec) || it->path().extension() != ".so") continue;
        entries_.emplace(it->path().filename().string(), it->path());
    }

    std::cout << "[PluginRegistry] discovered " << entries_.size()
              << " plugin(s) under " << pluginsDir << std::endl;
}

std::optional<std::filesystem::path> Registry::find(const std::string& pluginId) const {
    auto it = entries_.find(pluginId);
    if (it == entries_.end()) return std::nullopt;
    return it->second;
}

std::filesystem::path executableDirectory() {
    char buf[PATH_MAX];
    const ssize_t n = ::readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (n <= 0) {
        return std::filesystem::current_path();
    }
    buf[n] = '\0';
    return std::filesystem::path(buf).parent_path();
}

}
