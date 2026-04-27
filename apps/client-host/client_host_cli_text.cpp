// File: apps/client-host/client_host_cli_text.cpp
// Purpose: Application entry point or host support for BLITZAR executables.

#include "apps/client-host/client_host_cli_text.hpp"
#include <algorithm>
#include <cctype>
#include <utility>
namespace grav_client_host {
/// Description: Defines the ClientHostCliTextLocal data or behavior contract.
class ClientHostCliTextLocal final {
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
        std::string current;
        bool inQuotes = false;
        char quoteChar = '\0';
        for (char c : line) {
            if (!inQuotes && (c == '"' || c == '\'')) {
                inQuotes = true;
                quoteChar = c;
                continue;
            }
            if (inQuotes && c == quoteChar) {
                inQuotes = false;
                quoteChar = '\0';
                continue;
            }
            if (!inQuotes && std::isspace(static_cast<unsigned char>(c)) != 0) {
                if (!current.empty()) {
                    tokens.push_back(current);
                    current.clear();
                }
                continue;
            }
            current.push_back(c);
        }
        if (!current.empty()) {
            tokens.push_back(current);
        }
        return tokens;
    }
    /// Description: Executes the toLower operation.
    static std::string toLower(std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return value;
    }
};
/// Description: Executes the trim operation.
std::string ClientHostCliText::trim(const std::string& input)
{
    return ClientHostCliTextLocal::trim(input);
}
/// Description: Executes the splitTokens operation.
std::vector<std::string> ClientHostCliText::splitTokens(const std::string& line)
{
    return ClientHostCliTextLocal::splitTokens(line);
}
/// Description: Executes the toLower operation.
std::string ClientHostCliText::toLower(std::string value)
{
    return ClientHostCliTextLocal::toLower(std::move(value));
}
} // namespace grav_client_host
