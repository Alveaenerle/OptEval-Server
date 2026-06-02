#include "SessionLogger.hpp"

#include <fstream>
#include <iostream>

namespace archivist {

RunHandle SessionLogger::begin_run(const std::string& evalId,
                                   const std::string& problem_id) {
    uint64_t run;
    {
        std::lock_guard<std::mutex> lock(storage_mutex_);
        const std::string key = evalId + '\x1f' + problem_id;
        run = run_counts_[key]++;
    }

    const std::filesystem::path plugin_path = base_path_ / evalId / problem_id;
    try {
        std::filesystem::create_directories(plugin_path);
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "[SessionLogger] create_directories(" << plugin_path
                  << ") failed: " << e.what() << std::endl;
    }

    const std::filesystem::path file_path =
        plugin_path / ("run_" + std::to_string(run) + ".dat");
    std::ofstream ofs(file_path, std::ios::app);
    if (!ofs.is_open()) {
        std::cerr << "[SessionLogger] cannot open " << file_path << std::endl;
    }
    return RunHandle{run, std::move(ofs)};
}

}
