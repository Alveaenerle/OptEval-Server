#include "archivist/archivist_service.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
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

PlotOptions optionsFor(const optEval::PlotSpec& spec) {
    PlotOptions opts;
    opts.include_convergence = spec.include_convergence();
    opts.include_ecdf        = spec.include_ecdf();
    if (spec.convergence_windows() > 0) {
        opts.convergence_windows = spec.convergence_windows();
    }
    opts.ecdf_percentiles.assign(spec.ecdf_percentiles().begin(),
                                 spec.ecdf_percentiles().end());
    return opts;
}

}

grpc::Status ArchivistServiceImpl::CreateSession(
    grpc::ServerContext*,
    const optEval::CreateSessionRequest* request,
    optEval::SessionId* response) {

    std::string prefix = request->prefix();
    if (prefix.empty()) prefix = "eval";

    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
    localtime_r(&t, &tm);

    const uint64_t n = session_counter_.fetch_add(1, std::memory_order_relaxed);

    std::ostringstream oss;
    oss << prefix << '-' << std::put_time(&tm, "%Y%m%d-%H%M%S") << '-' << n;

    response->set_eval_id(oss.str());
    std::cout << "[Archivist] created session " << oss.str() << std::endl;
    return grpc::Status::OK;
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

    // One LogEvaluations stream == one independent run of this plugin. The
    // run's output file stays open for the whole stream (no per-value reopen).
    RunHandle handle = logger_.begin_run(eval_id, plugin_id);

    uint64_t count = 0;
    optEval::LogValue v;
    while (reader->Read(&v)) {
        handle.out << v.value() << "\n";
        ++count;
    }
    ack->set_count(count);
    std::cout << "[Archivist] logged " << count << " values for "
              << eval_id << "/" << plugin_id << " run_" << handle.run << std::endl;
    return grpc::Status::OK;
}

grpc::Status ArchivistServiceImpl::GetPlotData(
    grpc::ServerContext*,
    const optEval::PlotRequest* request,
    optEval::PlotData* response) {

    // Empty selection => every plugin in the session, both panels, defaults.
    if (request->plugins_size() == 0) {
        PlotOptions opts;  // defaults: both true, default windows/percentiles
        const auto all = buildPlots(request->eval_id(), "", opts, data_dir_);
        for (const auto& [name, pp] : all) emit(response, name, pp);
        return grpc::Status::OK;
    }

    // Per-plugin selection: each PlotSpec carries its own panel flags + knobs.
    for (const auto& spec : request->plugins()) {
        if (!spec.include_convergence() && !spec.include_ecdf()) continue;
        const PlotOptions opts = optionsFor(spec);
        const auto one = buildPlots(request->eval_id(), spec.plugin_id(),
                                    opts, data_dir_);
        for (const auto& [name, pp] : one) emit(response, name, pp);
    }
    return grpc::Status::OK;
}

}
