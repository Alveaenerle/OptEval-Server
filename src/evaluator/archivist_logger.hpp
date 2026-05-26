#pragma once

#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>

#include "archivist.grpc.pb.h"
#include "evaluator/evaluation_logger.hpp"

namespace evaluator {

// EvaluationLogger backed by a client-streaming RPC to the Archivist service.
// One Evaluate() session corresponds to one open LogEvaluations stream.
class ArchivistLogger : public EvaluationLogger {
public:
    explicit ArchivistLogger(std::shared_ptr<grpc::Channel> channel);

    std::unique_ptr<EvaluationSession> begin_session(
        const std::string& eval_id, const std::string& plugin_id) override;

private:
    std::unique_ptr<optEval::Archivist::Stub> stub_;
};

}
