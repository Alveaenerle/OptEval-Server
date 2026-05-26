#include "archivist/plot_handler.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <limits>

namespace archivist {

namespace {

// Fixed number of convergence bins. The client (matplotlib) controls the
// visual size of the plot; the server's job is just to emit a compact,
// renderable summary. 200 is enough for a smooth curve at any viewport size.
constexpr std::size_t kConvergenceWindows = 200;

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

// Bin best-so-far series into kConvergenceWindows windows, emit (x, mean) per
// window, then fold consecutive windows whose mean is exactly equal.
std::vector<ConvergencePoint> buildConvergence(const std::vector<double>& bsf) {
    std::vector<ConvergencePoint> out;
    if (bsf.empty()) return out;

    const std::size_t n = bsf.size();
    const std::size_t windows = std::min<std::size_t>(kConvergenceWindows, n);
    out.reserve(windows);

    for (std::size_t w = 0; w < windows; ++w) {
        const std::size_t begin = (w * n) / windows;
        const std::size_t end   = ((w + 1) * n) / windows;
        if (end == begin) continue;
        double sum = 0.0;
        for (std::size_t i = begin; i < end; ++i) sum += bsf[i];
        ConvergencePoint p;
        p.mean = sum / static_cast<double>(end - begin);
        p.x = static_cast<int>(end);  // evaluation index at the window's end
        if (!out.empty() && out.back().mean == p.mean) {
            out.back().x = p.x;  // extend the flat segment
        } else {
            out.push_back(p);
        }
    }
    return out;
}

// Sample fixed percentiles from the raw value distribution.
std::vector<EcdfPoint> buildEcdf(std::vector<double> raw) {
    std::vector<EcdfPoint> out;
    if (raw.empty()) return out;
    std::sort(raw.begin(), raw.end());
    out.reserve(kDefaultPercentiles.size());
    const double last_idx = static_cast<double>(raw.size() - 1);
    for (double p : kDefaultPercentiles) {
        const std::size_t idx = static_cast<std::size_t>(std::round(p * last_idx));
        out.push_back({p, raw[idx]});
    }
    return out;
}

PluginPlot buildOne(const std::filesystem::path& file_path, const PlotOptions& opts) {
    PluginPlot pp;
    const std::vector<double> raw = readRawValues(file_path);
    if (raw.empty()) return pp;
    if (opts.include_convergence) pp.convergence = buildConvergence(bestSoFar(raw));
    if (opts.include_ecdf)        pp.ecdf        = buildEcdf(raw);
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
        const std::filesystem::path fp = dir / (plugin_id + ".dat");
        if (std::filesystem::exists(fp)) {
            out.emplace(plugin_id, buildOne(fp, opts));
        }
        return out;
    }

    if (!std::filesystem::is_directory(dir)) return out;
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        if (entry.path().extension() != ".dat") continue;
        out.emplace(entry.path().stem().string(), buildOne(entry.path(), opts));
    }
    return out;
}

}
