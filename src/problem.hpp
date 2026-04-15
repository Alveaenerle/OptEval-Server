#pragma once
#include <dlfcn.h>

namespace plugin {

typedef int (*GetDimensionsFunc)();
typedef bool (*CheckConstraintsFunc)(const double*);
typedef double (*EvaluateFunc)(const double*);


class Problem {
public:
    explicit Problem(void* pluginHandle) : 
        get_dimensions_(reinterpret_cast<GetDimensionsFunc>(dlsym(pluginHandle, "get_dimensions"))),
        check_constraints_(reinterpret_cast<CheckConstraintsFunc>(dlsym(pluginHandle, "check_constraints"))),
        evaluate_(reinterpret_cast<EvaluateFunc>(dlsym(pluginHandle, "evaluate"))) {}

    ~Problem() {}
    Problem& operator=(const Problem&) = delete;

    GetDimensionsFunc get_dimensions_;
    CheckConstraintsFunc check_constraints_;
    EvaluateFunc evaluate_;
};

}