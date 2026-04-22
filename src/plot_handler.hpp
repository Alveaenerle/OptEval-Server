#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <cmath>
#include <limits>
#include <algorithm>

inline std::string buildPlotJson(const std::string& evalID, const std::string& pluginId) {
    std::string json = "{";
    std::filesystem::path dir = std::filesystem::path("results") / evalID;
    bool first_plugin = true;

    auto process_file = [&](const std::filesystem::path& file_path, const std::string& plugin_name) {
        if (!first_plugin) json += ",";
        first_plugin = false;

        std::ifstream ifile(file_path);
        std::vector<double> vals;
        std::vector<int> true_evals;
        double min_val = std::numeric_limits<double>::max();
        std::string token;
        int eval_idx = 0;
        
        while (ifile >> token) {
            eval_idx++;
            try {
                double v = std::stod(token);
                if (!std::isnan(v)) {
                    if (v < min_val) min_val = v;
                }
                if (min_val < std::numeric_limits<double>::max()) {
                    vals.push_back(min_val);
                    true_evals.push_back(eval_idx);
                }
            } catch (...) {
            }
        }
        
        if (vals.empty()) {
            json += "\"" + plugin_name + "\": {}";
            first_plugin = false;
            return;
        }

        std::vector<double> downsampled_y;
        std::vector<int> downsampled_x;
        if (vals.size() > 1000) {
            double step = (double)vals.size() / 1000.0;
            for (int i = 0; i < 1000; i++) {
                size_t idx = std::min((size_t)(i * step), vals.size() - 1);
                downsampled_y.push_back(vals[idx]);
                downsampled_x.push_back(true_evals[idx]);
            }
        } else {
            downsampled_y = vals;
            downsampled_x = true_evals;
        }

        std::vector<double> targets(51);
        for (int i = 0; i < 51; ++i) {
            targets[i] = std::pow(10.0, 2.0 - i * 0.2);
        }
        
        std::vector<int> evals_to_target;
        for (int i = 0; i < 51; ++i) {
            int found_idx = -1;
            for (size_t k = 0; k < vals.size(); ++k) {
                if (vals[k] <= targets[i]) {
                    found_idx = true_evals[k];
                    break;
                }
            }
            if (found_idx != -1) {
                evals_to_target.push_back(found_idx);
            }
        }
        
        std::sort(evals_to_target.begin(), evals_to_target.end());
        
        std::vector<double> ecdf_x;
        std::vector<double> ecdf_y;
        
        ecdf_x.push_back(1.0);
        ecdf_y.push_back(0.0);
        
        for (size_t i = 0; i < evals_to_target.size(); ++i) {
            ecdf_x.push_back(evals_to_target[i]);
            ecdf_y.push_back((double)(i + 1) / 51.0);
        }
        
        if (!ecdf_x.empty() && ecdf_x.back() < eval_idx) {
            ecdf_x.push_back(eval_idx);
            ecdf_y.push_back(ecdf_y.back());
        }

        json += "\"" + plugin_name + "\": {";
        
        json += "\"points_x\": [";
        for (size_t i = 0; i < downsampled_x.size(); i++) {
            if (i > 0) json += ",";
            json += std::to_string(downsampled_x[i]);
        }
        json += "],";

        json += "\"points\": [";
        for (size_t i = 0; i < downsampled_y.size(); i++) {
            if (i > 0) json += ",";
            json += std::to_string(downsampled_y[i]);
        }
        json += "],";

        json += "\"ecdf_x\": [";
        for (size_t i = 0; i < ecdf_x.size(); i++) {
            if (i > 0) json += ",";
            json += std::to_string(ecdf_x[i]);
        }
        json += "],";

        json += "\"ecdf_y\": [";
        for (size_t i = 0; i < ecdf_y.size(); i++) {
            if (i > 0) json += ",";
            json += std::to_string(ecdf_y[i]);
        }
        json += "]}";
    };

    if (!pluginId.empty()) {
        std::filesystem::path fp = dir / (pluginId + ".dat");
        if (std::filesystem::exists(fp)) {
             process_file(fp, pluginId);
        }
    } else {
        if (std::filesystem::exists(dir) && std::filesystem::is_directory(dir)) {
            for (const auto& entry : std::filesystem::directory_iterator(dir)) {
                 if (entry.path().extension() == ".dat") {
                     std::string p_name = entry.path().stem().string();
                     process_file(entry.path(), p_name);
                 }
            }
        }
    }

    json += "}";
    return json;
}
