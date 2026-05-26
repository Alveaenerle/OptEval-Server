#include "config.hpp"

#include <fstream>
#include <stdexcept>

#include <nlohmann/json.hpp>

namespace optEvalCfg {

namespace {

Role parseRole(const std::string& s) {
    if (s == "evaluator") return Role::Evaluator;
    if (s == "archivist") return Role::Archivist;
    if (s == "both")      return Role::Both;
    throw std::runtime_error("config.role must be one of: evaluator, archivist, both (got: " + s + ")");
}

template <typename T>
T getOr(const nlohmann::json& obj, const char* key, T fallback) {
    auto it = obj.find(key);
    if (it == obj.end() || it->is_null()) return fallback;
    return it->get<T>();
}

}

const char* roleToString(Role r) {
    switch (r) {
        case Role::Evaluator: return "evaluator";
        case Role::Archivist: return "archivist";
        case Role::Both:      return "both";
    }
    return "?";
}

Config loadConfig(const std::filesystem::path& path) {
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        throw std::runtime_error("Cannot open config file: " + path.string());
    }

    nlohmann::json j;
    try {
        ifs >> j;
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to parse JSON in " + path.string() + ": " + e.what());
    }

    Config c;
    c.role = parseRole(getOr<std::string>(j, "role", "both"));

    if (j.contains("evaluator")) {
        const auto& e = j["evaluator"];
        c.evaluator.listen                  = getOr<std::string>(e, "listen", c.evaluator.listen);
        c.evaluator.port                    = getOr<int>(e, "port", c.evaluator.port);
        c.evaluator.max_concurrent_sessions = getOr<int>(e, "max_concurrent_sessions",
                                                         c.evaluator.max_concurrent_sessions);
        c.evaluator.plugins_dir             = getOr<std::string>(e, "plugins_dir",
                                                                 c.evaluator.plugins_dir.string());
        c.evaluator.archivist_addr          = getOr<std::string>(e, "archivist_addr",
                                                                 c.evaluator.archivist_addr);
    }

    if (j.contains("archivist")) {
        const auto& a = j["archivist"];
        c.archivist.listen   = getOr<std::string>(a, "listen", c.archivist.listen);
        c.archivist.port     = getOr<int>(a, "port", c.archivist.port);
        c.archivist.data_dir = getOr<std::string>(a, "data_dir", c.archivist.data_dir.string());
    }

    if (c.evaluator.max_concurrent_sessions < 1) {
        throw std::runtime_error("evaluator.max_concurrent_sessions must be >= 1");
    }
    if (c.evaluator.port < 1 || c.evaluator.port > 65535 ||
        c.archivist.port < 1 || c.archivist.port > 65535) {
        throw std::runtime_error("port values must be in [1, 65535]");
    }

    return c;
}

}
