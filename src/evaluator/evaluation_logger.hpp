#pragma once

#include <memory>
#include <string>

namespace evaluator {

// One per-session sink. The evaluator service holds it for the lifetime of
// an Evaluate() stream and calls record() per evaluation.
class EvaluationSession {
public:
    virtual ~EvaluationSession() = default;
    virtual void record(double value) = 0;
};

// Factory for sessions. DIP seam: the evaluator service depends on this,
// not on the concrete gRPC client (or on no logger at all).
class EvaluationLogger {
public:
    virtual ~EvaluationLogger() = default;
    virtual std::unique_ptr<EvaluationSession> begin_session(
        const std::string& eval_id, const std::string& plugin_id) = 0;
};

class NullLogger : public EvaluationLogger {
public:
    std::unique_ptr<EvaluationSession> begin_session(const std::string&,
                                                     const std::string&) override {
        return std::make_unique<NullSession>();
    }

private:
    struct NullSession : EvaluationSession {
        void record(double) override {}
    };
};

}
