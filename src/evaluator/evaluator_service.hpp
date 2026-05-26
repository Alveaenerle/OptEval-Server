#pragma once

#include <grpcpp/grpcpp.h>

#include "evaluator.grpc.pb.h"
#include "evaluator/evaluation_logger.hpp"

namespace evaluator {

class EvaluatorServiceImpl final : public optEval::Evaluator::Service {
public:
    explicit EvaluatorServiceImpl(EvaluationLogger& logger) : logger_(logger) {}

    grpc::Status Evaluate(
        grpc::ServerContext* ctx,
        grpc::ServerReaderWriter<optEval::EvalResult, optEval::EvalVector>* stream) override;

private:
    EvaluationLogger& logger_;
};

}
