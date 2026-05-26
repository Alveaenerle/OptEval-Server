#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>

#include "plugin.hpp"

namespace eval {

enum StatusFlag : uint8_t {
    NAN_VALUE_RETURNED      = 1 << 0,
    BUDGET_EXHAUSTED        = 1 << 1,
    TARGET_REACHED          = 1 << 2,
    EVALUATION_DENIED       = 1 << 3,
    DIMENSION_MISMATCH      = 1 << 4,
    OUT_OF_BOUND_VIOLATION  = 1 << 5,
    PLUGIN_RUNTIME_ERROR    = 1 << 6,
    UNSUPPORTED_PLUGIN_ID   = 1 << 7,
};

struct Result {
    uint8_t status = 0;
    double value = std::numeric_limits<double>::quiet_NaN();
};

inline Result evaluate(const plugin::Plugin& p, const double* vector, std::size_t n) {
    Result out;
    if (n != static_cast<std::size_t>(p->get_dimensions_())) {
        out.status |= DIMENSION_MISMATCH;
        return out;
    }
    if (!p->check_constraints_(vector)) {
        out.status |= OUT_OF_BOUND_VIOLATION;
        return out;
    }
    out.value = p->evaluate_(vector);
    return out;
}

}
