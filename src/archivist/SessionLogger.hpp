#pragma once

#include <filesystem>
#include <mutex>
#include <string>

namespace archivist {

class SessionLogger {
public:
    explicit SessionLogger(std::filesystem::path base_path)
        : base_path_(std::move(base_path)) {}

    // Ensures `<base>/<evalId>/` exists. Idempotent.
    void initialize_session(const std::string& evalId);

    // Appends `value\n` to `<base>/<evalId>/<problem_id>.dat`. Thread-safe.
    void log_evaluation(const std::string& evalId,
                        const std::string& problem_id,
                        double value);

    const std::filesystem::path& base_path() const { return base_path_; }

private:
    std::mutex storage_mutex_;
    std::filesystem::path base_path_;
};

}
