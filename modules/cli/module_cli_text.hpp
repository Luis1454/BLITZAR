// File: modules/cli/module_cli_text.hpp
// Purpose: Client module implementation for BLITZAR extension workflows.

#ifndef GRAVITY_MODULES_CLI_MODULE_CLI_TEXT_HPP_
#define GRAVITY_MODULES_CLI_MODULE_CLI_TEXT_HPP_
#include <string>
#include <vector>
namespace grav_module_cli {
/// Description: Defines the ModuleCliText data or behavior contract.
class ModuleCliText final {
public:
    /// Description: Executes the trim operation.
    static std::string trim(const std::string& input);
    /// Description: Executes the splitTokens operation.
    static std::vector<std::string> splitTokens(const std::string& line);
};
} // namespace grav_module_cli
#endif // GRAVITY_MODULES_CLI_MODULE_CLI_TEXT_HPP_
