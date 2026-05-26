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
};

// Reads .dat files under <data_dir>/<eval_id>/ and produces a compressed
// representation per plugin. If plugin_id is non-empty, only that plugin
// is considered; otherwise all .dat files in the session directory are.
//
// Convergence and ECDF are computed from disjoint data shapes by design:
//   - convergence: window-mean over the best-so-far trajectory, adjacent
//     duplicate-mean windows folded.
//   - ecdf:        fixed-percentile samples over the raw value distribution.
// They are never derived from each other.
std::map<std::string, PluginPlot> buildPlots(const std::string& eval_id,
                                             const std::string& plugin_id,
                                             const PlotOptions& opts,
                                             const std::filesystem::path& data_dir);

}
