#pragma once

#include <atomic>
#include <cstdint>
#include <filesystem>

#include <grpcpp/grpcpp.h>

#include "archivist.grpc.pb.h"
#include "archivist/SessionLogger.hpp"

namespace archivist {

class ArchivistServiceImpl final : public optEval::Archivist::Service {
public:
    ArchivistServiceImpl(SessionLogger& logger, std::filesystem::path data_dir)
        : logger_(logger), data_dir_(std::move(data_dir)) {}

    grpc::Status CreateSession(grpc::ServerContext* ctx,
                               const optEval::CreateSessionRequest* request,
                               optEval::SessionId* response) override;

    grpc::Status LogEvaluations(grpc::ServerContext* ctx,
                                grpc::ServerReader<optEval::LogValue>* reader,
                                optEval::LogAck* ack) override;

    grpc::Status GetPlotData(grpc::ServerContext* ctx,
                             const optEval::PlotRequest* request,
                             optEval::PlotData* response) override;

private:
    SessionLogger& logger_;
    std::filesystem::path data_dir_;
    std::atomic<uint64_t> session_counter_{0};
};

}
