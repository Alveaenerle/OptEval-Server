#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>

namespace plugin {

class Registry {
public:
    static Registry& instance();

    void scan(const std::filesystem::path& pluginsDir);
    std::optional<std::filesystem::path> find(const std::string& pluginId) const;
    std::size_t size() const { return entries_.size(); }

private:
    Registry() = default;
    std::unordered_map<std::string, std::filesystem::path> entries_;
};

std::filesystem::path executableDirectory();

}
