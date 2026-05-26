#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "archivist/run.hpp"
#include "config.hpp"
#include "evaluator/run.hpp"
#include "lifecycle.hpp"

namespace {

struct CliArgs {
    std::string config_path = "server-config.json";
};

CliArgs parseCli(int argc, char** argv) {
    CliArgs a;
    for (int i = 1; i < argc; ++i) {
        const std::string s = argv[i];
        const auto eq = s.find('=');
        if (eq == std::string::npos) continue;
        const std::string key = s.substr(0, eq);
        const std::string val = s.substr(eq + 1);
        if (key == "--config") a.config_path = val;
    }
    return a;
}

}

// The "master" thread:
//   1. parses --config=path,
//   2. reads JSON config,
//   3. spawns evaluator and/or archivist on their own threads,
//   4. joins them and exits.
// Signals (SIGINT/SIGTERM) trigger a graceful shutdown of both services.
int main(int argc, char** argv) {
    const CliArgs cli = parseCli(argc, argv);

    optEvalCfg::Config cfg;
    try {
        cfg = optEvalCfg::loadConfig(cli.config_path);
    } catch (const std::exception& e) {
        std::cerr << "[Main] config error: " << e.what() << std::endl;
        std::cerr << "Usage: opt_eval_server --config=path/to/server-config.json" << std::endl;
        return 1;
    }

    std::cout << "[Main] role=" << optEvalCfg::roleToString(cfg.role) << std::endl;
    lifecycle::install_signal_handlers();

    std::vector<std::thread> workers;

    const bool want_archivist = cfg.role == optEvalCfg::Role::Archivist ||
                                cfg.role == optEvalCfg::Role::Both;
    const bool want_evaluator = cfg.role == optEvalCfg::Role::Evaluator ||
                                cfg.role == optEvalCfg::Role::Both;

    if (want_archivist) {
        workers.emplace_back([cfg = cfg.archivist]() { archivist::run(cfg); });
    }
    if (want_evaluator) {
        // In role=Both, force evaluator -> local archivist regardless of config,
        // so users don't have to keep two ports in sync.
        optEvalCfg::EvaluatorConfig ec = cfg.evaluator;
        if (cfg.role == optEvalCfg::Role::Both) {
            ec.archivist_addr = "localhost:" + std::to_string(cfg.archivist.port);
        }
        workers.emplace_back([ec]() { evaluator::run(ec); });
    }

    for (auto& t : workers) t.join();
    std::cout << "[Main] all services exited; goodbye." << std::endl;
    return 0;
}
