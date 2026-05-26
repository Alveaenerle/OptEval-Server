#include "archivist/run.hpp"

#include <iostream>
#include <memory>

#include <grpcpp/grpcpp.h>

#include "archivist/SessionLogger.hpp"
#include "archivist/archivist_service.hpp"
#include "lifecycle.hpp"

namespace archivist {

int run(const optEvalCfg::ArchivistConfig& cfg) {
    SessionLogger logger(cfg.data_dir);
    ArchivistServiceImpl service(logger, cfg.data_dir);

    const std::string addr = cfg.listen + ":" + std::to_string(cfg.port);
    grpc::ServerBuilder builder;
    builder.AddListeningPort(addr, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::shared_ptr<grpc::Server> server(builder.BuildAndStart());
    if (!server) {
        std::cerr << "[Archivist] Failed to bind " << addr << std::endl;
        return 1;
    }
    lifecycle::register_server(server);
    std::cout << "[Archivist] Listening on " << addr
              << " (data_dir=" << cfg.data_dir << ")" << std::endl;
    server->Wait();
    std::cout << "[Archivist] Shut down." << std::endl;
    return 0;
}

}
