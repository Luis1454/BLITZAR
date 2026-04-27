// File: engine/src/config/DirectiveValueFormatter.cpp
// Purpose: Engine implementation for the BLITZAR simulation core.

#include "config/DirectiveValueFormatter.hpp"
#include <algorithm>
#include <cctype>
namespace grav_config {
/// Description: Executes the quote operation.
std::string DirectiveValueFormatter::quote(const std::string& value)
{
    if (value.empty()) {
        return "\"\"";
    }
    const bool needsQuotes = std::any_of(value.begin(), value.end(), [](unsigned char c) {
        return std::isspace(c) != 0 || c == ',' || c == '(' || c == ')' || c == '"' || c == '\'';
    });
    if (!needsQuotes) {
        return value;
    }
    std::string quoted;
    quoted.reserve(value.size() + 2u);
    quoted.push_back('"');
    for (char c : value) {
        if (c == '"' || c == '\\') {
            quoted.push_back('\\');
        }
        quoted.push_back(c);
    }
    quoted.push_back('"');
    return quoted;
}
} // namespace grav_config
