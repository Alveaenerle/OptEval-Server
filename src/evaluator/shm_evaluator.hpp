#pragma once

#include <atomic>
#include <string>

#include "config.hpp"
#include "evaluator/evaluation_logger.hpp"

namespace evaluator::shm {

// Blocking. Accept-loops the SHM connect region and spawns one detached
// session thread per Evaluate session. Returns when the global lifecycle
// shutdown flag flips, or on a fatal setup error.
int runServer(const optEvalCfg::EvaluatorConfig& cfg, EvaluationLogger& logger);

}
