#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <mutex>
#include <string>

namespace archivist {

// One independent run's open output file. The caller streams values straight
// into `out` for the lifetime of the LogEvaluations stream; closing it (on
// destruction) flushes the run. Keeping the stream open avoids reopening the
// file per value, which is O(evaluations) syscalls under the global lock.
struct RunHandle {
    uint64_t run = 0;
    std::ofstream out;
};

class SessionLogger {
public:
    explicit SessionLogger(std::filesystem::path base_path)
        : base_path_(std::move(base_path)) {}

    // Allocates the next independent-run index for this (evalId, problem_id),
    // ensures `<base>/<evalId>/<problem_id>/` exists, and returns its open
    // `run_<run>.dat` stream. One call per LogEvaluations stream => one run.
    // Only the run-index allocation is locked; writing to the returned stream
    // is lock-free. Thread-safe.
    RunHandle begin_run(const std::string& evalId, const std::string& problem_id);

    const std::filesystem::path& base_path() const { return base_path_; }

private:
    std::mutex storage_mutex_;
    std::filesystem::path base_path_;
    // (evalId + '\x1f' + problem_id) -> number of runs started so far.
    std::map<std::string, uint64_t> run_counts_;
};

}
