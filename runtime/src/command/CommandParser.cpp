// File: runtime/src/command/CommandParser.cpp
// Purpose: Runtime integration surface for BLITZAR clients and protocols.

#include "command/CommandParser.hpp"
#include "command/CommandCatalog.hpp"
#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdlib>
namespace grav_cmd {
/// Description: Executes the trimTokenLine operation.
static std::string trimTokenLine(const std::string& input)
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
/// Description: Executes the stripComment operation.
static std::string stripComment(const std::string& line)
{
    std::string output;
    bool inQuotes = false;
    char quoteChar = '\0';
    for (char c : line) {
        if (!inQuotes && (c == '"' || c == '\'')) {
            inQuotes = true;
            quoteChar = c;
            output.push_back(c);
            continue;
        }
        if (inQuotes && c == quoteChar) {
            inQuotes = false;
            quoteChar = '\0';
            output.push_back(c);
            continue;
        }
        if (!inQuotes && c == '#') {
            break;
        }
        output.push_back(c);
    }
    return output;
}
/// Description: Executes the parseUintToken operation.
static bool parseUintToken(const std::string& token, std::uint64_t& outValue)
{
    const char* begin = token.data();
    const char* end = token.data() + token.size();
    const std::from_chars_result result = std::from_chars(begin, end, outValue);
    return result.ec == std::errc() && result.ptr == end;
}
/// Description: Executes the parseFloatToken operation.
static bool parseFloatToken(const std::string& token, double& outValue)
{
    char* end = nullptr;
    outValue = std::strtod(token.c_str(), &end);
    return end != nullptr && *end == '\0';
}
static CommandParseResult parseTokens(const std::vector<std::string>& tokens,
                                      std::size_t lineNumber)
{
    CommandParseResult result{};
    if (tokens.empty()) {
        result.ok = true;
        return result;
    }
    const CommandSpec* spec = CommandCatalog::findByName(tokens.front());
    if (spec == nullptr) {
        result.error =
            "line " + std::to_string(lineNumber) + ": unknown command '" + tokens.front() + "'";
        return result;
    }
    std::size_t required = 0u;
    for (const CommandArgumentSpec& argument : spec->arguments)
        if (!argument.optional) {
            required += 1u;
        }
    const std::size_t provided = tokens.size() - 1u;
    if (provided < required || provided > spec->arguments.size()) {
        result.error =
            "line " + std::to_string(lineNumber) + ": wrong arity for '" + spec->name + "'";
        return result;
    }
    CommandRequest request{};
    request.id = spec->id;
    request.name = spec->name;
    request.lineNumber = lineNumber;
    for (std::size_t index = 0u; index < provided; ++index) {
        const std::string& token = tokens[index + 1u];
        const CommandArgumentSpec& argument = spec->arguments[index];
        if (argument.kind == CommandArgumentKind::Uint ||
            argument.kind == CommandArgumentKind::Port) {
            std::uint64_t value = 0u;
            if (!parseUintToken(token, value)) {
                result.error =
                    "line " + std::to_string(lineNumber) + ": invalid integer '" + token + "'";
                return result;
            }
            request.arguments.push_back(value);
            continue;
        }
        if (argument.kind == CommandArgumentKind::Float) {
            double value = 0.0;
            if (!parseFloatToken(token, value)) {
                result.error =
                    "line " + std::to_string(lineNumber) + ": invalid float '" + token + "'";
                return result;
            }
            request.arguments.push_back(value);
            continue;
        }
        request.arguments.push_back(token);
    }
    result.ok = true;
    result.requests.push_back(std::move(request));
    return result;
}
/// Description: Executes the parseScript operation.
CommandParseResult CommandParser::parseScript(const std::string& scriptText)
{
    CommandParseResult result{};
    result.ok = true;
    std::size_t lineNumber = 1u;
    std::string current;
    for (char c : scriptText) {
        if (c == '\n') {
            CommandParseResult lineResult = parseLine(current, lineNumber);
            if (!lineResult.ok) {
                return lineResult;
            }
            if (!lineResult.requests.empty()) {
                result.requests.push_back(std::move(lineResult.requests.front()));
            }
            current.clear();
            lineNumber += 1u;
            continue;
        }
        current.push_back(c);
    }
    CommandParseResult tailResult = parseLine(current, lineNumber);
    if (!tailResult.ok) {
        return tailResult;
    }
    if (!tailResult.requests.empty()) {
        result.requests.push_back(std::move(tailResult.requests.front()));
    }
    return result;
}
/// Description: Executes the parseLine operation.
CommandParseResult CommandParser::parseLine(const std::string& line, std::size_t lineNumber)
{
    const std::string stripped = trimTokenLine(stripComment(line));
    if (stripped.empty()) {
        CommandParseResult result{};
        result.ok = true;
        return result;
    }
    return parseTokens(splitTokens(stripped), lineNumber);
}
} // namespace grav_cmd
