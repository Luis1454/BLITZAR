/*
 * @file modules/cli/module_cli_text.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Command-line client module for runtime control workflows.
 */

#ifndef GRAVITY_MODULES_CLI_MODULE_CLI_TEXT_HPP_
#define GRAVITY_MODULES_CLI_MODULE_CLI_TEXT_HPP_
#include <string>
#include <vector>

namespace grav_module_cli {
class ModuleCliText final {
public:
    static std::string trim(const std::string& input);
    static std::vector<std::string> splitTokens(const std::string& line);
};
} // namespace grav_module_cli
#endif // GRAVITY_MODULES_CLI_MODULE_CLI_TEXT_HPP_
