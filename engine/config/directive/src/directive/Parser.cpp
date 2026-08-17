/*
 * @file engine/config/directive/src/directive/Parser.cpp
 * @brief Lexical parsing for directive lines.
 */

#include "DirectiveInternals.hpp"

#include <algorithm>
#include <cctype>

namespace bltzr_config {
static std::string trimDirective(std::string_view value)
{
    const auto begin = std::find_if_not(value.begin(), value.end(), [](unsigned char c) {
        return std::isspace(c) != 0;
    });
    const auto end = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char c) {
                         return std::isspace(c) != 0;
                     }).base();
    if (begin >= end) {
        return {};
    }
    return std::string(begin, end);
}

static std::string unquoteDirective(std::string value)
{
    if (value.size() >= 2u) {
        const char first = value.front();
        const char last = value.back();
        if ((first == '"' && last == '"') || (first == '\'' && last == '\'')) {
            return value.substr(1u, value.size() - 2u);
        }
    }
    return value;
}

static bool appendArgument(std::string_view raw, DirectiveArguments& args)
{
    const std::string entry = trimDirective(raw);
    if (entry.empty()) {
        return true;
    }
    const std::size_t equal = entry.find('=');
    if (equal == std::string::npos || equal == 0u) {
        return false;
    }
    args.emplace_back(trimDirective(entry.substr(0u, equal)),
                      unquoteDirective(trimDirective(entry.substr(equal + 1u))));
    return true;
}

bool parseDirective(std::string_view raw, std::string& name, DirectiveArguments& args)
{
    const std::string stripped = trimDirective(raw);
    const std::size_t open = stripped.find('(');
    const std::size_t close = stripped.rfind(')');
    if (open == std::string::npos || close != stripped.size() - 1u || open == 0u) {
        return false;
    }

    name = trimDirective(stripped.substr(0u, open));
    args.clear();
    const std::string_view body = std::string_view(stripped).substr(open + 1u, close - open - 1u);
    std::string token;
    char quote = '\0';
    for (char character : body) {
        if (quote != '\0') {
            if (character == quote) {
                quote = '\0';
            }
            token.push_back(character);
            continue;
        }
        if (character == '"' || character == '\'') {
            quote = character;
            token.push_back(character);
            continue;
        }
        if (character == ',') {
            if (!appendArgument(token, args)) {
                return false;
            }
            token.clear();
            continue;
        }
        token.push_back(character);
    }
    return appendArgument(token, args);
}
} // namespace bltzr_config
