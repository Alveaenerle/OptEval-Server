#include "archivist/plot_handler.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <limits>

namespace archivist {

namespace {

// Default convergence resolution when the request leaves it at 0.
constexpr int kDefaultConvergenceWindows = 200;

// Tail-dense percentiles for the value-distribution ECDF.
constexpr std::array<double, 13> kDefaultPercentiles = {
    0.001, 0.005, 0.01, 0.05, 0.10, 0.25, 0.50, 0.75, 0.90, 0.95, 0.99, 0.995, 0.999
};

std::vector<double> readRawValues(const std::filesystem::path& file_path) {
    std::vector<double> out;
    std::ifstream ifs(file_path);
    if (!ifs.is_open()) return out;
    std::string token;
    while (ifs >> token) {
        try {
            const double v = std::stod(token);
            if (!std::isnan(v)) out.push_back(v);
        } catch (...) {
        }
    }
    return out;
}

std::vector<double> bestSoFar(const std::vector<double>& raw) {
    std::vector<double> out;
    out.reserve(raw.size());
    double best = std::numeric_limits<double>::max();
    for (double v : raw) {
        if (v < best) best = v;
        out.push_back(best);
    }
    return out;
}

// Average best-so-far across runs. Each run's best-so-far series is binned into
// `windows` equal index-fraction windows; the per-window means (and window-end
// indices) are then averaged across all runs. Adjacent windows whose averaged
// mean is exactly equal are folded.
//
// Every run contributes a value to *every* window: when a run has fewer
// evaluations than there are windows a bin can be empty, in which case we
// sample the nearest best-so-far point instead of skipping the window. Skipping
// is what produced the sawtooth — with more windows than evaluations each
// window's bin was non-empty for only one run, so adjacent windows averaged
// different subsets of runs and the curve alternated between their values
// instead of being the smooth, monotone average of all runs.
std::vector<ConvergencePoint> buildConvergence(
    const std::vector<std::vector<double>>& runs, int windows) {
    std::vector<ConvergencePoint> out;
    const std::size_t W = windows > 0 ? static_cast<std::size_t>(windows)
                                      : kDefaultConvergenceWindows;

    std::vector<double> mean_sum(W, 0.0);
    std::vector<double> x_sum(W, 0.0);
    std::vector<std::size_t> count(W, 0);

    for (const auto& raw : runs) {
        if (raw.empty()) continue;
        const std::vector<double> bsf = bestSoFar(raw);
        const std::size_t n = bsf.size();
        for (std::size_t w = 0; w < W; ++w) {
            const std::size_t begin = (w * n) / W;
            const std::size_t end   = ((w + 1) * n) / W;
            double mean;
            std::size_t x_idx;  // 1-based evaluation count this window represents
            if (end > begin) {
                double sum = 0.0;
                for (std::size_t i = begin; i < end; ++i) sum += bsf[i];
                mean  = sum / static_cast<double>(end - begin);
                x_idx = end;
            } else {
                const std::size_t idx = begin < n ? begin : n - 1;
                mean  = bsf[idx];
                x_idx = idx + 1;
            }
            mean_sum[w] += mean;
            x_sum[w]    += static_cast<double>(x_idx);
            ++count[w];
        }
    }

    out.reserve(W);
    for (std::size_t w = 0; w < W; ++w) {
        if (count[w] == 0) continue;
        ConvergencePoint p;
        p.mean = mean_sum[w] / static_cast<double>(count[w]);
        p.x = static_cast<int>(std::round(x_sum[w] / static_cast<double>(count[w])));
        if (!out.empty() && out.back().mean == p.mean) {
            out.back().x = p.x;  // extend the flat segment
        } else {
            out.push_back(p);
        }
    }
    return out;
}

// Sample percentiles from the pooled value distribution.
std::vector<EcdfPoint> buildEcdf(std::vector<double> pooled,
                                 const std::vector<double>& percentiles) {
    std::vector<EcdfPoint> out;
    if (pooled.empty()) return out;
    std::sort(pooled.begin(), pooled.end());

    const double* pcts = percentiles.empty() ? kDefaultPercentiles.data()
                                             : percentiles.data();
    const std::size_t npct = percentiles.empty() ? kDefaultPercentiles.size()
                                                  : percentiles.size();

    out.reserve(npct);
    const double last_idx = static_cast<double>(pooled.size() - 1);
    for (std::size_t i = 0; i < npct; ++i) {
        double p = pcts[i];
        if (p < 0.0) p = 0.0;
        if (p > 1.0) p = 1.0;
        const std::size_t idx = static_cast<std::size_t>(std::round(p * last_idx));
        out.push_back({p, pooled[idx]});
    }
    return out;
}

// Collect the raw values of every independent run of one plugin. New layout:
// <plugin_path> is a directory of run_*.dat. Legacy layout: a flat .dat file.
std::vector<std::vector<double>> readRuns(const std::filesystem::path& plugin_path) {
    std::vector<std::vector<double>> runs;
    if (std::filesystem::is_directory(plugin_path)) {
        std::vector<std::filesystem::path> files;
        for (const auto& entry : std::filesystem::directory_iterator(plugin_path)) {
            if (entry.path().extension() == ".dat") files.push_back(entry.path());
        }
        std::sort(files.begin(), files.end());
        for (const auto& f : files) runs.push_back(readRawValues(f));
    } else if (std::filesystem::is_regular_file(plugin_path)) {
        runs.push_back(readRawValues(plugin_path));
    }
    return runs;
}

PluginPlot buildOne(const std::filesystem::path& plugin_path, const PlotOptions& opts) {
    PluginPlot pp;
    const std::vector<std::vector<double>> runs = readRuns(plugin_path);
    if (runs.empty()) return pp;

    if (opts.include_convergence) {
        pp.convergence = buildConvergence(runs, opts.convergence_windows);
    }
    if (opts.include_ecdf) {
        std::vector<double> pooled;
        for (const auto& r : runs) pooled.insert(pooled.end(), r.begin(), r.end());
        pp.ecdf = buildEcdf(std::move(pooled), opts.ecdf_percentiles);
    }
    return pp;
}

}

std::map<std::string, PluginPlot> buildPlots(const std::string& eval_id,
                                             const std::string& plugin_id,
                                             const PlotOptions& opts,
                                             const std::filesystem::path& data_dir) {
    std::map<std::string, PluginPlot> out;
    const std::filesystem::path dir = data_dir / eval_id;

    if (!plugin_id.empty()) {
        const std::filesystem::path plugin_dir = dir / plugin_id;
        if (std::filesystem::is_directory(plugin_dir)) {
            out.emplace(plugin_id, buildOne(plugin_dir, opts));
        } else {
            const std::filesystem::path legacy = dir / (plugin_id + ".dat");
            if (std::filesystem::exists(legacy)) {
                out.emplace(plugin_id, buildOne(legacy, opts));
            }
        }
        return out;
    }

    if (!std::filesystem::is_directory(dir)) return out;
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        if (entry.is_directory()) {
            out.emplace(entry.path().filename().string(), buildOne(entry.path(), opts));
        } else if (entry.path().extension() == ".dat") {
            out.emplace(entry.path().stem().string(), buildOne(entry.path(), opts));
        }
    }
    return out;
}

}
