#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace archivist {

// One point on the per-window mean of the best-so-far convergence trajectory.
struct ConvergencePoint {
    int x;        // evaluation index at the end of the window
    double mean;  // mean of the best-so-far series within that window
};

// One sample on the value-distribution ECDF: P(X <= value) == percentile.
struct EcdfPoint {
    double percentile;  // in [0, 1]
    double value;
};

struct PluginPlot {
    std::vector<ConvergencePoint> convergence;
    std::vector<EcdfPoint> ecdf;
};

struct PlotOptions {
    bool include_convergence = true;
    bool include_ecdf = true;
    // Curve resolution for convergence binning.
    int convergence_windows = 200;
    // Explicit ECDF sample points; empty => the built-in tail-dense set.
    std::vector<double> ecdf_percentiles;
};

// Reads the per-plugin run files under <data_dir>/<eval_id>/<plugin_id>/run_*.dat
// (legacy flat <plugin_id>.dat is also accepted) and produces a compressed
// representation per plugin. If plugin_id is non-empty, only that plugin is
// considered; otherwise every plugin directory in the session is.
//
// Across the independent runs of a plugin:
//   - convergence: per-run best-so-far is binned, then averaged window-by-window.
//   - ecdf:        all runs' raw values are pooled into one distribution.
// Convergence and ECDF are computed from disjoint data shapes by design and are
// never derived from each other.
std::map<std::string, PluginPlot> buildPlots(const std::string& eval_id,
                                             const std::string& plugin_id,
                                             const PlotOptions& opts,
                                             const std::filesystem::path& data_dir);

}
