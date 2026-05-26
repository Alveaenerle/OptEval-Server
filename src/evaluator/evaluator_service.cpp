#include "evaluator/evaluator_service.hpp"

#include <iostream>
#include <memory>
#include <string>

#include "evaluator/evaluator.hpp"
#include "evaluator/plugin.hpp"

namespace evaluator {

namespace {

std::string readMetadata(const grpc::ServerContext& ctx, const std::string& key) {
    const auto& md = ctx.client_metadata();
    auto it = md.find(key);
    if (it == md.end()) return {};
    return std::string(it->second.data(), it->second.size());
}

}

grpc::Status EvaluatorServiceImpl::Evaluate(
    grpc::ServerContext* ctx,
    grpc::ServerReaderWriter<optEval::EvalResult, optEval::EvalVector>* stream) {

    const std::string plugin_id = readMetadata(*ctx, "plugin-id");
    const std::string eval_id   = readMetadata(*ctx, "eval-id");
    if (plugin_id.empty()) {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                            "missing 'plugin-id' metadata");
    }

    std::unique_ptr<plugin::Plugin> plugin;
    try {
        plugin = std::make_unique<plugin::Plugin>(plugin_id);
    } catch (const std::exception& e) {
        return grpc::Status(grpc::StatusCode::NOT_FOUND, e.what());
    }
    std::cout << "[Evaluator] session start eval_id=" << eval_id
              << " plugin_id=" << plugin_id << std::endl;

    auto session = logger_.begin_session(eval_id, plugin_id);

    optEval::EvalVector req;
    while (stream->Read(&req)) {
        const eval::Result r = eval::evaluate(
            *plugin, req.values().data(), static_cast<std::size_t>(req.values_size()));

        optEval::EvalResult resp;
        resp.set_status(r.status);
        resp.set_value(r.value);
        stream->Write(resp);

        if (!eval_id.empty()) session->record(r.value);
    }
    std::cout << "[Evaluator] session end plugin_id=" << plugin_id << std::endl;
    return grpc::Status::OK;
}

}
