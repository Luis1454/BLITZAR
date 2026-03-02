#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "frontend/FrontendModuleHandle.hpp"

namespace grav_module_host {

class ModuleHostModuleOps final {
public:
    static std::vector<std::filesystem::path> buildSearchRoots(std::string_view programName);
    static std::string resolveModuleSpecifier(
        const std::string &rawSpecifier,
        const std::vector<std::filesystem::path> &searchRoots);
    static bool switchModule(
        const std::string &moduleSpecifier,
        const std::string &configPath,
        const std::vector<std::filesystem::path> &searchRoots,
        grav_module::FrontendModuleHandle &module);
    static bool reloadModule(
        const std::string &currentModuleSpecifier,
        const std::string &configPath,
        const std::vector<std::filesystem::path> &searchRoots,
        grav_module::FrontendModuleHandle &module);
};

} // namespace grav_module_host
