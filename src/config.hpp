#pragma once

#include <filesystem>
#include <string>

namespace optEvalCfg {

enum class Role { Evaluator, Archivist, Both };

struct EvaluatorConfig {
    std::string listen = "0.0.0.0";
    int port = 50051;
    // Hard cap on concurrent Evaluate() sessions. Each bidi-stream session
    // consumes one gRPC sync-server handler thread for its lifetime.
    int max_concurrent_sessions = 16;
    // Resolved relative to the executable directory if not absolute.
    std::filesystem::path plugins_dir = "plugins";
    // gRPC endpoint of the archivist. In role=Both this is auto-rewritten
    // to "localhost:<archivist.port>" before evaluator::run is called.
    std::string archivist_addr = "localhost:50052";
};

struct ArchivistConfig {
    std::string listen = "0.0.0.0";
    int port = 50052;
    // Where SessionLogger writes .dat files and where plot_handler reads them.
    std::filesystem::path data_dir = "results";
};

struct Config {
    Role role = Role::Both;
    EvaluatorConfig evaluator;
    ArchivistConfig archivist;
};

// Reads JSON from `path`, validates, returns a fully-populated Config.
// Throws std::runtime_error on any parse / validation failure.
Config loadConfig(const std::filesystem::path& path);

const char* roleToString(Role r);

}
