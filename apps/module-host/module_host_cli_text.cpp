#include <algorithm>
#include <cctype>
#include <utility>

#include "apps/module-host/module_host_cli_text.hpp"

namespace grav_module_host {

class ModuleHostCliTextLocal final {
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

    static std::string toLower(std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        return value;
    }
};

std::string ModuleHostCliText::trim(const std::string &input)
{
    return ModuleHostCliTextLocal::trim(input);
}

std::vector<std::string> ModuleHostCliText::splitTokens(const std::string &line)
{
    return ModuleHostCliTextLocal::splitTokens(line);
}

std::string ModuleHostCliText::toLower(std::string value)
{
    return ModuleHostCliTextLocal::toLower(std::move(value));
}

} // namespace grav_module_host
