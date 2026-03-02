#pragma once

#include <string>
#include <vector>

namespace grav_module_cli {

class ModuleCliText final {
public:
    static std::string trim(const std::string &input);
    static std::vector<std::string> splitTokens(const std::string &line);
};

} // namespace grav_module_cli
