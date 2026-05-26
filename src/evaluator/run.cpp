#include "evaluator/run.hpp"

#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>
#include <grpcpp/resource_quota.h>

#include "evaluator/archivist_logger.hpp"
#include "evaluator/evaluation_logger.hpp"
#include "evaluator/evaluator_service.hpp"
#include "evaluator/plugin_registry.hpp"
#include "lifecycle.hpp"

namespace evaluator {

namespace {

std::filesystem::path resolvePluginsDir(const std::filesystem::path& configured) {
    if (configured.is_absolute()) return configured;
    return plugin::executableDirectory() / configured;
}

}

int run(const optEvalCfg::EvaluatorConfig& cfg) {
    const auto pluginsDir = resolvePluginsDir(cfg.plugins_dir);
    plugin::Registry::instance().scan(pluginsDir);

    std::unique_ptr<EvaluationLogger> logger;
    if (cfg.archivist_addr.empty()) {
        std::cout << "[Evaluator] No archivist address; running with NullLogger." << std::endl;
        logger = std::make_unique<NullLogger>();
    } else {
        std::cout << "[Evaluator] Logging to archivist at " << cfg.archivist_addr << std::endl;
        logger = std::make_unique<ArchivistLogger>(
            grpc::CreateChannel(cfg.archivist_addr, grpc::InsecureChannelCredentials()));
    }

    EvaluatorServiceImpl service(*logger);

    const std::string addr = cfg.listen + ":" + std::to_string(cfg.port);
    grpc::ServerBuilder builder;
    builder.AddListeningPort(addr, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    // Cap concurrent in-flight RPCs (each Evaluate bidi-stream session uses
    // one sync-server handler thread for its lifetime).
    const int max_sessions = cfg.max_concurrent_sessions;
    grpc::ResourceQuota quota("evaluator_quota");
    quota.SetMaxThreads(max_sessions + 4);  // small headroom for completion-queue threads
    builder.SetResourceQuota(quota);
    using SyncOpt = grpc::ServerBuilder::SyncServerOption;
    builder.SetSyncServerOption(SyncOpt::MIN_POLLERS, 1);
    builder.SetSyncServerOption(SyncOpt::MAX_POLLERS, max_sessions);

    std::shared_ptr<grpc::Server> server(builder.BuildAndStart());
    if (!server) {
        std::cerr << "[Evaluator] Failed to bind " << addr << std::endl;
        return 1;
    }
    lifecycle::register_server(server);
    std::cout << "[Evaluator] Listening on " << addr
              << " (max_concurrent_sessions=" << max_sessions << ")" << std::endl;
    server->Wait();
    std::cout << "[Evaluator] Shut down." << std::endl;
    return 0;
}

}
