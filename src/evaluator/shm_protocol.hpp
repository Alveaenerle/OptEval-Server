#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

// Wire format for SHM-transport Evaluator. Both the C++ server and the Python
// client mmap these structs over files in /dev/shm. Layout must stay
// trivially-copyable, with std::atomic<uint32_t> fields being layout-compatible
// with a plain uint32_t. Synchronization is busy-spin on the atomic flags.
namespace evaluator::shm {

inline constexpr std::size_t MAX_DIM = 4096;
inline constexpr std::size_t PLUGIN_ID_LEN = 256;
inline constexpr std::size_t EVAL_ID_LEN = 256;
inline constexpr std::size_t SESSION_NAME_LEN = 96;
inline constexpr std::size_t ERROR_LEN = 256;

// Handshake region. One per server, fixed name `<shm_id>_connect`.
// Clients contend for it via fcntl(F_OFD_SETLKW) on the file itself; only
// the lock holder may write into client_request / read server_ack.
struct ConnectRegion {
    std::atomic<uint32_t> client_request;     // 0 = idle, 1 = pending
    std::atomic<uint32_t> server_ack;          // 0 = idle, 1 = response ready
    uint32_t              session_status;      // 0 = ok, non-zero = error
    uint32_t              _pad0;
    char                  plugin_id[PLUGIN_ID_LEN];
    char                  eval_id[EVAL_ID_LEN];
    char                  session_name[SESSION_NAME_LEN];  // basename under /dev/shm
    char                  session_error[ERROR_LEN];
};

// One per active Evaluate session. Name is `<shm_id>_session_<pid>_<n>`.
// Lockless: a single producer (client) and single consumer (server thread).
struct SessionRegion {
    std::atomic<uint32_t> request_ready;       // client -> server
    std::atomic<uint32_t> response_ready;      // server -> client
    std::atomic<uint32_t> shutdown;            // client closes session
    uint32_t              dim;                 // # doubles in `values`
    uint32_t              status;              // response status bitmask
    uint32_t              _pad0;
    double                value;               // response value
    double                values[MAX_DIM];     // request vector
};

static_assert(sizeof(std::atomic<uint32_t>) == sizeof(uint32_t),
              "std::atomic<uint32_t> must be layout-compatible with uint32_t "
              "for Python ctypes interop");
static_assert(std::atomic<uint32_t>::is_always_lock_free,
              "Need lock-free 32-bit atomics for the SHM protocol");

}
