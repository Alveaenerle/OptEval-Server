#pragma once
#include <string>
#include <dlfcn.h>
#include <iostream>
#include "problem.hpp"
#include "plugin_utils.hpp"


namespace plugin {

class Plugin {
public:
    explicit Plugin(const std::string &pluginId) : 
        pluginHandle(plugin::loadPlugin(pluginId)),
        problem(pluginHandle) {}
    ~Plugin() {
        if (pluginHandle) {
            dlclose(pluginHandle);
        }
    }
    Plugin(const Plugin&) = delete;
    Plugin& operator=(const Plugin&) = delete;
    const Problem* operator->() const {
        return &problem;
    }


private:
    void* pluginHandle;
    const Problem problem;
};

}