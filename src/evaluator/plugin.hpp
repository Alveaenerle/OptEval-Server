#pragma once

#include <dlfcn.h>
#include <string>

#include "plugin_utils.hpp"
#include "problem.hpp"

namespace plugin {

class Plugin {
public:
    explicit Plugin(const std::string& pluginId)
        : pluginHandle_(plugin::loadPlugin(pluginId)),
          problem_(pluginHandle_) {}

    ~Plugin() {
        if (pluginHandle_) {
            ::dlclose(pluginHandle_);
        }
    }

    Plugin(const Plugin&) = delete;
    Plugin& operator=(const Plugin&) = delete;

    const Problem* operator->() const { return &problem_; }

private:
    void* pluginHandle_;
    Problem problem_;
};

}
