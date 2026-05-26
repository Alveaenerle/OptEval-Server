#include "archivist/archivist_service.hpp"

#include <iostream>
#include <string>

#include "archivist/plot_handler.hpp"

namespace archivist {

namespace {

std::string readMetadata(const grpc::ServerContext& ctx, const std::string& key) {
    const auto& md = ctx.client_metadata();
    auto it = md.find(key);
    if (it == md.end()) return {};
    return std::string(it->second.data(), it->second.size());
}

void emit(optEval::PlotData* response,
          const std::string& name,
          const PluginPlot& pp) {
    auto& pb = (*response->mutable_plugins())[name];
    for (const auto& cp : pp.convergence) {
        auto* pt = pb.add_convergence();
        pt->set_x(cp.x);
        pt->set_mean(cp.mean);
    }
    for (const auto& ep : pp.ecdf) {
        auto* pt = pb.add_ecdf();
        pt->set_percentile(ep.percentile);
        pt->set_value(ep.value);
    }
}

}

grpc::Status ArchivistServiceImpl::LogEvaluations(
    grpc::ServerContext* ctx,
    grpc::ServerReader<optEval::LogValue>* reader,
    optEval::LogAck* ack) {

    const std::string eval_id   = readMetadata(*ctx, "eval-id");
    const std::string plugin_id = readMetadata(*ctx, "plugin-id");
    if (eval_id.empty() || plugin_id.empty()) {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                            "missing 'eval-id' or 'plugin-id' metadata");
    }

    logger_.initialize_session(eval_id);

    uint64_t count = 0;
    optEval::LogValue v;
    while (reader->Read(&v)) {
        logger_.log_evaluation(eval_id, plugin_id, v.value());
        ++count;
    }
    ack->set_count(count);
    std::cout << "[Archivist] logged " << count << " values for "
              << eval_id << "/" << plugin_id << std::endl;
    return grpc::Status::OK;
}

grpc::Status ArchivistServiceImpl::GetPlotData(
    grpc::ServerContext*,
    const optEval::PlotRequest* request,
    optEval::PlotData* response) {

    // Empty selection => every plugin in the session, both panels.
    if (request->plugins_size() == 0) {
        PlotOptions opts;  // defaults: both true
        const auto all = buildPlots(request->eval_id(), "", opts, data_dir_);
        for (const auto& [name, pp] : all) emit(response, name, pp);
        return grpc::Status::OK;
    }

    // Per-plugin selection: each PlotSpec defines its own panel flags.
    for (const auto& spec : request->plugins()) {
        if (!spec.include_convergence() && !spec.include_ecdf()) continue;
        PlotOptions opts;
        opts.include_convergence = spec.include_convergence();
        opts.include_ecdf        = spec.include_ecdf();
        const auto one = buildPlots(request->eval_id(), spec.plugin_id(),
                                    opts, data_dir_);
        for (const auto& [name, pp] : one) emit(response, name, pp);
    }
    return grpc::Status::OK;
}

}
