#include "evaluator/archivist_logger.hpp"

#include <iostream>

namespace evaluator {

namespace {

class ArchivistSession : public EvaluationSession {
public:
    ArchivistSession(optEval::Archivist::Stub* stub,
                     const std::string& eval_id,
                     const std::string& plugin_id) {
        ctx_.AddMetadata("eval-id", eval_id);
        ctx_.AddMetadata("plugin-id", plugin_id);
        writer_ = stub->LogEvaluations(&ctx_, &ack_);
    }

    ~ArchivistSession() override {
        if (!writer_) return;
        writer_->WritesDone();
        const grpc::Status s = writer_->Finish();
        if (!s.ok()) {
            std::cerr << "[ArchivistLogger] LogEvaluations finished with error: "
                      << s.error_message() << std::endl;
        }
    }

    void record(double value) override {
        optEval::LogValue v;
        v.set_value(value);
        if (!writer_->Write(v)) {
            std::cerr << "[ArchivistLogger] Write failed (stream closed)" << std::endl;
        }
    }

private:
    grpc::ClientContext ctx_;
    optEval::LogAck ack_;
    std::unique_ptr<grpc::ClientWriter<optEval::LogValue>> writer_;
};

}

ArchivistLogger::ArchivistLogger(std::shared_ptr<grpc::Channel> channel)
    : stub_(optEval::Archivist::NewStub(std::move(channel))) {}

std::unique_ptr<EvaluationSession> ArchivistLogger::begin_session(
    const std::string& eval_id, const std::string& plugin_id) {
    return std::make_unique<ArchivistSession>(stub_.get(), eval_id, plugin_id);
}

}
