#pragma once
#include <dlfcn.h>
#include <stdexcept>
#include <string>

namespace plugin {

using GetDimensionsFunc = int (*)();
using CheckConstraintsFunc = bool (*)(const double*);
using EvaluateFunc = double (*)(const double*);

class Problem {
public:
    explicit Problem(void* pluginHandle)
        : get_dimensions_(resolve<GetDimensionsFunc>(pluginHandle, "get_dimensions")),
          check_constraints_(resolve<CheckConstraintsFunc>(pluginHandle, "check_constraints")),
          evaluate_(resolve<EvaluateFunc>(pluginHandle, "evaluate")) {}

    Problem(const Problem&) = delete;
    Problem& operator=(const Problem&) = delete;

    GetDimensionsFunc get_dimensions_;
    CheckConstraintsFunc check_constraints_;
    EvaluateFunc evaluate_;

private:
    template <typename F>
    static F resolve(void* handle, const char* symbol) {
        ::dlerror();
        auto fn = reinterpret_cast<F>(::dlsym(handle, symbol));
        if (const char* err = ::dlerror()) {
            throw std::runtime_error(std::string("Missing symbol '") + symbol + "': " + err);
        }
        return fn;
    }
};

}
