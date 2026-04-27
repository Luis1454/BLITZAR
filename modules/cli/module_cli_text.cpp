// File: modules/cli/module_cli_text.cpp
// Purpose: Client module implementation for BLITZAR extension workflows.

#include "modules/cli/module_cli_text.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>
#include <vector>
namespace grav_module_cli {
/// Description: Defines the ModuleCliTextLocal data or behavior contract.
class ModuleCliTextLocal final {
public:
    /// Description: Executes the trim operation.
    static std::string trim(const std::string& input)
    {
        const auto begin = std::find_if_not(input.begin(), input.end(),
                                            [](unsigned char c) { return std::isspace(c) != 0; });
        const auto end = std::find_if_not(input.rbegin(), input.rend(), [](unsigned char c) {
                             return std::isspace(c) != 0;
                         }).base();
        if (begin >= end)
            return {};
        return std::string(begin, end);
    }
    /// Description: Executes the splitTokens operation.
    static std::vector<std::string> splitTokens(const std::string& line)
    {
        std::vector<std::string> tokens;
        /// Description: Executes the input operation.
        std::istringstream input(line);
        std::string token;
        while (input >> token) {
            tokens.push_back(token);
        }
        return tokens;
    }
};
/// Description: Executes the trim operation.
std::string ModuleCliText::trim(const std::string& input)
{
    return ModuleCliTextLocal::trim(input);
}
/// Description: Executes the splitTokens operation.
std::vector<std::string> ModuleCliText::splitTokens(const std::string& line)
{
    return ModuleCliTextLocal::splitTokens(line);
}
} // namespace grav_module_cli
