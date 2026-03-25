#pragma once
#include <string>
#include <dlfcn.h>
#include <stdexcept>
#include <iostream>
#include "problem.hpp"


class Plugin {
public:
    Plugin(const std::string &pluginPath) : 
        pluginHandle(loadPlugin(pluginPath)),
        problem(pluginHandle) {}
    ~Plugin() {
        if (pluginHandle) {
            dlclose(pluginHandle);
        }
    }
    const Problem& get() const {
        return problem;
    }

private:
    void* pluginHandle;
    Problem problem;
    void* loadPlugin(const std::string &pluginPath) const {
        void* pluginHandle = dlopen(pluginPath.c_str(), RTLD_LAZY);
        if (!pluginHandle) {
            throw std::runtime_error(std::string("Error loading .so: ") + dlerror());
        }

        std::cout << "[Server] Plugin loaded.\n";
        return pluginHandle;
    }
};