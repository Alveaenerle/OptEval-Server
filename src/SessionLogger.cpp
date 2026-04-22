#include "SessionLogger.hpp"
#include <iostream>

SessionLogger global_logger;

void SessionLogger::initialize_session(const std::string& evalId) {
    std::lock_guard<std::mutex> lock(storage_mutex);
    std::filesystem::path session_path = base_path / evalId;
    
    try {
        if (!std::filesystem::exists(session_path)) {
            std::filesystem::create_directories(session_path);
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "[SessionLogger] Error creating directories: " << e.what() << std::endl;
    }
}

void SessionLogger::log_evaluation(const std::string& evalId, const std::string& problem_id, double value) {
    std::lock_guard<std::mutex> lock(storage_mutex);
    std::filesystem::path session_path = base_path / evalId;
    std::filesystem::path file_path = session_path / (problem_id + ".dat");
    
    // Safely ensure directory exists if missing, though initialize_session should be called first
    try {
        if (!std::filesystem::exists(session_path)) {
            std::filesystem::create_directories(session_path);
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "[SessionLogger] Error creating directories during logging: " << e.what() << std::endl;
    }

    std::ofstream ofs(file_path, std::ios::app);
    if (!ofs.is_open()) {
        std::cerr << "[SessionLogger] Error opening file for writing: " << file_path << std::endl;
        return;
    }
    
    ofs << value << "\n";
}