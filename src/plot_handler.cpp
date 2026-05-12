#include "plot_handler.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

namespace {

constexpr std::size_t kMaxPlotPoints = 1000;
constexpr int kTargetCount = 51;

struct Trace {
    std::vector<int> points_x;
    std::vector<double> points;
    std::vector<double> ecdf_x;
    std::vector<double> ecdf_y;
};

template <typename T>
void appendArray(std::ostringstream& out, const std::vector<T>& values) {
    out << '[';
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i > 0) out << ',';
        out << std::to_string(values[i]);
    }
    out << ']';
}

std::vector<double> computeTargets() {
    std::vector<double> targets(kTargetCount);
    for (int i = 0; i < kTargetCount; ++i) {
        targets[i] = std::pow(10.0, 2.0 - i * 0.2);
    }
    return targets;
}

void downsample(const std::vector<double>& vals,
                const std::vector<int>& true_evals,
                std::vector<int>& out_x,
                std::vector<double>& out_y) {
    if (vals.size() <= kMaxPlotPoints) {
        out_x = true_evals;
        out_y = vals;
        return;
    }
    const double step = static_cast<double>(vals.size()) / kMaxPlotPoints;
    out_x.reserve(kMaxPlotPoints);
    out_y.reserve(kMaxPlotPoints);
    for (std::size_t i = 0; i < kMaxPlotPoints; ++i) {
        const std::size_t idx = std::min(static_cast<std::size_t>(i * step), vals.size() - 1);
        out_x.push_back(true_evals[idx]);
        out_y.push_back(vals[idx]);
    }
}

bool readBestSoFar(const std::filesystem::path& file_path,
                   std::vector<double>& vals,
                   std::vector<int>& true_evals,
                   int& total_evals) {
    std::ifstream ifs(file_path);
    if (!ifs.is_open()) return false;

    double current_min = std::numeric_limits<double>::max();
    std::string token;
    total_evals = 0;
    while (ifs >> token) {
        ++total_evals;
        try {
            const double v = std::stod(token);
            if (!std::isnan(v) && v < current_min) current_min = v;
        } catch (...) {
        }
        if (current_min < std::numeric_limits<double>::max()) {
            vals.push_back(current_min);
            true_evals.push_back(total_evals);
        }
    }
    return true;
}

void buildEcdf(const std::vector<double>& vals,
               const std::vector<int>& true_evals,
               int total_evals,
               std::vector<double>& ecdf_x,
               std::vector<double>& ecdf_y) {
    const std::vector<double> targets = computeTargets();
    std::vector<int> evals_to_target;
    evals_to_target.reserve(kTargetCount);

    for (double target : targets) {
        for (std::size_t k = 0; k < vals.size(); ++k) {
            if (vals[k] <= target) {
                evals_to_target.push_back(true_evals[k]);
                break;
            }
        }
    }
    std::sort(evals_to_target.begin(), evals_to_target.end());

    ecdf_x.push_back(1.0);
    ecdf_y.push_back(0.0);
    for (std::size_t i = 0; i < evals_to_target.size(); ++i) {
        ecdf_x.push_back(evals_to_target[i]);
        ecdf_y.push_back(static_cast<double>(i + 1) / kTargetCount);
    }
    if (!ecdf_x.empty() && ecdf_x.back() < total_evals) {
        ecdf_x.push_back(total_evals);
        ecdf_y.push_back(ecdf_y.back());
    }
}

bool buildTrace(const std::filesystem::path& file_path, Trace& trace) {
    std::vector<double> vals;
    std::vector<int> true_evals;
    int total_evals = 0;
    if (!readBestSoFar(file_path, vals, true_evals, total_evals)) return false;
    if (vals.empty()) return false;

    downsample(vals, true_evals, trace.points_x, trace.points);
    buildEcdf(vals, true_evals, total_evals, trace.ecdf_x, trace.ecdf_y);
    return true;
}

void writeTraceJson(std::ostringstream& out, const std::string& name, const Trace& trace) {
    out << '"' << name << "\": {";
    out << "\"points_x\": "; appendArray(out, trace.points_x); out << ',';
    out << "\"points\": ";   appendArray(out, trace.points);   out << ',';
    out << "\"ecdf_x\": ";   appendArray(out, trace.ecdf_x);   out << ',';
    out << "\"ecdf_y\": ";   appendArray(out, trace.ecdf_y);
    out << '}';
}

void emitFile(std::ostringstream& out,
              const std::filesystem::path& file_path,
              const std::string& name,
              bool& first) {
    Trace trace;
    const bool ok = buildTrace(file_path, trace);

    if (!first) out << ',';
    first = false;

    if (!ok) {
        out << '"' << name << "\": {}";
        return;
    }
    writeTraceJson(out, name, trace);
}

}

std::string buildPlotJson(const std::string& evalId, const std::string& pluginId) {
    std::ostringstream out;
    out << '{';

    const std::filesystem::path dir = std::filesystem::path("results") / evalId;
    bool first = true;

    if (!pluginId.empty()) {
        const std::filesystem::path fp = dir / (pluginId + ".dat");
        if (std::filesystem::exists(fp)) {
            emitFile(out, fp, pluginId, first);
        }
    } else if (std::filesystem::is_directory(dir)) {
        for (const auto& entry : std::filesystem::directory_iterator(dir)) {
            if (entry.path().extension() != ".dat") continue;
            emitFile(out, entry.path(), entry.path().stem().string(), first);
        }
    }

    out << '}';
    return out.str();
}
