/*
 * @file modules/cli/Text.cpp
 * @author BLITZAR Contributors
 * @project BLITZAR
 * @brief Command-line client module for runtime control workflows.
 */

#include "modules/cli/Text.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>
#include <vector>

namespace bltzr_module_cli {
std::string trim(const std::string& input)
{
    const auto begin = std::find_if_not(input.begin(), input.end(), [](unsigned char c)
        { return std::isspace(c) != 0; });

    const auto end = std::find_if_not(input.rbegin(), input.rend(), [](unsigned char c)
        { return std::isspace(c) != 0; }).base();

    if (begin >= end)
        return {};
    return std::string(begin, end);
}

std::vector<std::string> splitTokens(const std::string& line)
{
    std::vector<std::string> tokens;
    std::istringstream input(line);
    std::string token;

    while (input >> token) {
        tokens.push_back(token);
    }
    return tokens;
}
} // namespace bltzr_module_cli
