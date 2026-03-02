#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>
#include <vector>

#include "modules/cli/module_cli_text.hpp"

namespace grav_module_cli {

class ModuleCliTextLocal final {
public:
    static std::string trim(const std::string &input)
    {
        const auto begin = std::find_if_not(input.begin(), input.end(), [](unsigned char c) {
            return std::isspace(c) != 0;
        });
        const auto end = std::find_if_not(input.rbegin(), input.rend(), [](unsigned char c) {
            return std::isspace(c) != 0;
        }).base();
        if (begin >= end) {
            return {};
        }
        return std::string(begin, end);
    }

    static std::vector<std::string> splitTokens(const std::string &line)
    {
        std::vector<std::string> tokens;
        std::istringstream input(line);
        std::string token;
        while (input >> token) {
            tokens.push_back(token);
        }
        return tokens;
    }
};

std::string ModuleCliText::trim(const std::string &input)
{
    return ModuleCliTextLocal::trim(input);
}

std::vector<std::string> ModuleCliText::splitTokens(const std::string &line)
{
    return ModuleCliTextLocal::splitTokens(line);
}

} // namespace grav_module_cli
