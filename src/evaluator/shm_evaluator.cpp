#include "evaluator/shm_evaluator.hpp"

#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "evaluator/evaluator.hpp"
#include "evaluator/plugin.hpp"
#include "evaluator/shm_protocol.hpp"
#include "lifecycle.hpp"

namespace evaluator::shm {

namespace {

constexpr auto kPollSleep = std::chrono::microseconds(0);

void copyTruncated(char* dst, std::size_t cap, const std::string& src) {
    const std::size_t n = std::min(cap - 1, src.size());
    std::memcpy(dst, src.data(), n);
    dst[n] = '\0';
}

// Pin the lock holder to the file itself with an exclusive OFD lock that
// covers the whole region. Returns the fd; caller closes it (lock auto-drops).
int acquireConnectFd(const std::string& connect_path, std::size_t size) {
    int fd = ::open(connect_path.c_str(), O_RDWR | O_CREAT, 0600);
    if (fd < 0) {
        std::cerr << "[SHM-Evaluator] open(" << connect_path << ") failed: "
                  << std::strerror(errno) << std::endl;
        return -1;
    }
    if (::ftruncate(fd, static_cast<off_t>(size)) != 0) {
        std::cerr << "[SHM-Evaluator] ftruncate failed: "
                  << std::strerror(errno) << std::endl;
        ::close(fd);
        return -1;
    }
    return fd;
}

// Map an existing-or-just-created region. Caller must munmap.
void* mapRegion(int fd, std::size_t size) {
    void* p = ::mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (p == MAP_FAILED) {
        std::cerr << "[SHM-Evaluator] mmap failed: "
                  << std::strerror(errno) << std::endl;
        return nullptr;
    }
    return p;
}

void runSession(std::string session_name,
                std::string shm_dir_path,
                std::unique_ptr<plugin::Plugin> plugin,
                std::unique_ptr<EvaluationSession> log_session,
                std::string eval_id,
                std::string plugin_id) {
    const std::string path = shm_dir_path + "/" + session_name;
    const std::size_t size = sizeof(SessionRegion);

    int fd = ::open(path.c_str(), O_RDWR, 0600);
    if (fd < 0) {
        std::cerr << "[SHM-Evaluator] session open(" << path << ") failed: "
                  << std::strerror(errno) << std::endl;
        return;
    }
    void* raw = mapRegion(fd, size);
    ::close(fd);
    if (!raw) return;

    auto* region = static_cast<SessionRegion*>(raw);

    std::cout << "[SHM-Evaluator] session start name=" << session_name
              << " eval_id=" << eval_id << " plugin_id=" << plugin_id << std::endl;

    while (!lifecycle::is_shutting_down()) {
        if (region->shutdown.load(std::memory_order_acquire) != 0) break;
        if (region->request_ready.load(std::memory_order_acquire) == 0) {
            if (kPollSleep.count() > 0) std::this_thread::sleep_for(kPollSleep);
            continue;
        }

        const uint32_t dim = region->dim;
        const eval::Result r = eval::evaluate(
            *plugin, region->values,
            static_cast<std::size_t>(dim > MAX_DIM ? MAX_DIM : dim));

        region->status = r.status;
        region->value  = r.value;
        // Publish response, then clear the request slot.
        region->response_ready.store(1, std::memory_order_release);
        region->request_ready.store(0, std::memory_order_release);

        if (!eval_id.empty() && log_session) log_session->record(r.value);
    }

    std::cout << "[SHM-Evaluator] session end name=" << session_name
              << " plugin_id=" << plugin_id << std::endl;

    ::munmap(raw, size);
    ::unlink(path.c_str());
}

}

int runServer(const optEvalCfg::EvaluatorConfig& cfg, EvaluationLogger& logger) {
    const std::string shm_dir = "/dev/shm";
    const std::string connect_path = shm_dir + "/" + cfg.shm_id + "_connect";

    int connect_fd = acquireConnectFd(connect_path, sizeof(ConnectRegion));
    if (connect_fd < 0) return 1;

    void* raw = mapRegion(connect_fd, sizeof(ConnectRegion));
    if (!raw) {
        ::close(connect_fd);
        return 1;
    }
    auto* connect = static_cast<ConnectRegion*>(raw);

    // Initialize: zero everything so a stale region from a previous run
    // doesn't trick a client into thinking there's a pending ack.
    std::memset(connect, 0, sizeof(ConnectRegion));

    std::cout << "[SHM-Evaluator] Listening on " << connect_path
              << " (session SHM under " << shm_dir << "/" << cfg.shm_id
              << "_session_*)" << std::endl;

    const pid_t pid = ::getpid();
    std::atomic<uint64_t> session_counter{0};

    while (!lifecycle::is_shutting_down()) {
        if (connect->client_request.load(std::memory_order_acquire) == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        const std::string plugin_id(connect->plugin_id);
        const std::string eval_id(connect->eval_id);

        // Allocate a session SHM up front so the client can open it as soon
        // as it sees server_ack.
        const uint64_t n = session_counter.fetch_add(1, std::memory_order_relaxed);
        const std::string session_basename =
            cfg.shm_id + "_session_" + std::to_string(pid) + "_" + std::to_string(n);
        const std::string session_path = shm_dir + "/" + session_basename;

        std::string err;
        std::unique_ptr<plugin::Plugin> plugin_inst;
        try {
            plugin_inst = std::make_unique<plugin::Plugin>(plugin_id);
        } catch (const std::exception& e) {
            err = std::string("plugin load failed: ") + e.what();
        }

        if (err.empty()) {
            int sfd = ::open(session_path.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0600);
            if (sfd < 0) {
                err = std::string("open session shm failed: ") + std::strerror(errno);
            } else {
                if (::ftruncate(sfd, static_cast<off_t>(sizeof(SessionRegion))) != 0) {
                    err = std::string("ftruncate session shm failed: ") + std::strerror(errno);
                } else {
                    // Zero-init via a brief mmap so client reads consistent state.
                    void* sraw = mapRegion(sfd, sizeof(SessionRegion));
                    if (!sraw) {
                        err = "mmap session shm failed";
                    } else {
                        std::memset(sraw, 0, sizeof(SessionRegion));
                        ::munmap(sraw, sizeof(SessionRegion));
                    }
                }
                ::close(sfd);
            }
        }

        if (err.empty()) {
            auto log_session = logger.begin_session(eval_id, plugin_id);
            std::thread(runSession,
                        session_basename, shm_dir,
                        std::move(plugin_inst), std::move(log_session),
                        eval_id, plugin_id).detach();
            copyTruncated(connect->session_name, SESSION_NAME_LEN, session_basename);
            connect->session_status = 0;
            connect->session_error[0] = '\0';
        } else {
            std::cerr << "[SHM-Evaluator] connect failed: " << err << std::endl;
            connect->session_name[0] = '\0';
            connect->session_status = 1;
            copyTruncated(connect->session_error, ERROR_LEN, err);
        }

        // Hand back to client and wait until it consumes.
        connect->server_ack.store(1, std::memory_order_release);
        while (connect->client_request.load(std::memory_order_acquire) != 0 &&
               !lifecycle::is_shutting_down()) {
            std::this_thread::sleep_for(std::chrono::microseconds(50));
        }
        connect->server_ack.store(0, std::memory_order_release);
    }

    std::cout << "[SHM-Evaluator] Shut down." << std::endl;
    ::munmap(raw, sizeof(ConnectRegion));
    ::close(connect_fd);
    ::unlink(connect_path.c_str());
    return 0;
}

}
