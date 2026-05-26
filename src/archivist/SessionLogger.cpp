#include "SessionLogger.hpp"

#include <fstream>
#include <iostream>

namespace archivist {

void SessionLogger::initialize_session(const std::string& evalId) {
    std::lock_guard<std::mutex> lock(storage_mutex_);
    const std::filesystem::path session_path = base_path_ / evalId;
    try {
        std::filesystem::create_directories(session_path);
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "[SessionLogger] create_directories(" << session_path
                  << ") failed: " << e.what() << std::endl;
    }
}

void SessionLogger::log_evaluation(const std::string& evalId,
                                   const std::string& problem_id,
                                   double value) {
    std::lock_guard<std::mutex> lock(storage_mutex_);
    const std::filesystem::path session_path = base_path_ / evalId;
    const std::filesystem::path file_path = session_path / (problem_id + ".dat");

    try {
        std::filesystem::create_directories(session_path);
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "[SessionLogger] create_directories(" << session_path
                  << ") failed: " << e.what() << std::endl;
    }

    std::ofstream ofs(file_path, std::ios::app);
    if (!ofs.is_open()) {
        std::cerr << "[SessionLogger] cannot open " << file_path << std::endl;
        return;
    }
    ofs << value << "\n";
}

}
