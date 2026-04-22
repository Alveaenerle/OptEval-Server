#pragma once

#include <string>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <map>

class SessionLogger {
public:
    // Ensures the base './results/' and the specific 'evalId' directory exist.
    // Should be called once per new experiment session.
    void initialize_session(const std::string& evalId);

    // Appends the double value to './results/{evalId}/{problem_id}.dat'.
    // Must be thread-safe.
    void log_evaluation(const std::string& evalId, const std::string& problem_id, double value);

private:
    std::mutex storage_mutex;
    std::filesystem::path base_path = "results";
};

// Expose a unified global instance to be used across all instances and threads
extern SessionLogger global_logger;