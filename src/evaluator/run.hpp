#pragma once

#include "config.hpp"

namespace evaluator {

// Blocks until the gRPC server shuts down. Returns process exit code.
int run(const optEvalCfg::EvaluatorConfig& cfg);

}
