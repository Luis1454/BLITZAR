#include "simulation/config/SimConfigFile.hpp"

#include <cstdint>
#include <fstream>
#include <iterator>
#include <new>
#include <stdexcept>
#include <utility>

namespace blitzar_sim {

namespace {

constexpr std::size_t MaxSourceBytes = 1024U * 1024U;
constexpr std::size_t MaxDirectives = 4096U;
constexpr std::size_t MaxArguments = 128U;
constexpr std::size_t MaxValueBytes = 4096U;

[[nodiscard]] bool IsIdentifierStart(char value) noexcept
{
    return (value >= 'a' && value <= 'z') || (value >= 'A' && value <= 'Z') || value == '_';
}

[[nodiscard]] bool IsIdentifierPart(char value) noexcept
{
    return IsIdentifierStart(value) || (value >= '0' && value <= '9') || value == '.' ||
           value == '-';
}

void SkipWhitespace(std::string_view line, std::size_t& cursor) noexcept
{
    while (cursor < line.size() &&
           (line[cursor] == ' ' || line[cursor] == '\t' || line[cursor] == '\r')) {
        ++cursor;
    }
}

[[nodiscard]] bool ReadIdentifier(
    std::string_view line, std::size_t& cursor, std::string& destination)
{
    if (cursor >= line.size() || !IsIdentifierStart(line[cursor])) {
        return false;
    }

    const std::size_t start = cursor++;

    while (cursor < line.size() && IsIdentifierPart(line[cursor])) {
        ++cursor;
    }

    destination.assign(line.substr(start, cursor - start));

    return true;
}

[[nodiscard]] bool ReadQuotedValue(
    std::string_view line, std::size_t& cursor, SimConfigFile::Value& destination)
{
    destination.quoted = true;

    destination.text.clear();

    ++cursor;

    while (cursor < line.size()) {
        const char value = line[cursor++];

        if (value == '"') {
            return true;
        }

        if (value == '\\') {
            if (cursor >= line.size()) {
                return false;
            }

            const char escaped = line[cursor++];

            switch (escaped) {
            case '"':
            case '\\':

                destination.text.push_back(escaped);

                break;

            case 'n':

                destination.text.push_back('\n');

                break;

            case 'r':

                destination.text.push_back('\r');

                break;

            case 't':

                destination.text.push_back('\t');

                break;

            default:

                return false;
            }
        }
        else {
            destination.text.push_back(value);
        }

        if (destination.text.size() > MaxValueBytes) {
            return false;
        }
    }

    return false;
}

[[nodiscard]] bool ReadTokenValue(
    std::string_view line, std::size_t& cursor, SimConfigFile::Value& destination)
{
    destination.quoted = false;

    const std::size_t start = cursor;

    while (cursor < line.size() && (IsIdentifierPart(line[cursor]) || line[cursor] == '+')) {
        ++cursor;
    }

    if (cursor == start || cursor - start > MaxValueBytes) {
        return false;
    }

    destination.text.assign(line.substr(start, cursor - start));

    return true;
}

[[nodiscard]] bool ReadValue(
    std::string_view line, std::size_t& cursor, SimConfigFile::Value& destination)
{
    if (cursor >= line.size()) {
        return false;
    }

    if (line[cursor] == '"') {
        return ReadQuotedValue(line, cursor, destination);
    }

    return ReadTokenValue(line, cursor, destination);
}

[[nodiscard]] bool ReadArgument(
    std::string_view line, std::size_t& cursor, SimConfigFile::Argument& destination)
{
    if (!ReadIdentifier(line, cursor, destination.name)) {
        return false;
    }

    SkipWhitespace(line, cursor);

    if (cursor >= line.size() || line[cursor] != '=') {
        return false;
    }

    ++cursor;

    SkipWhitespace(line, cursor);

    return ReadValue(line, cursor, destination.value);
}

[[nodiscard]] bool ReadDirective(
    std::string_view line, std::size_t& cursor, SimConfigFile::Directive& destination)
{
    if (!ReadIdentifier(line, cursor, destination.name)) {
        return false;
    }

    SkipWhitespace(line, cursor);

    if (cursor >= line.size() || line[cursor] != '(') {
        return false;
    }

    ++cursor;

    SkipWhitespace(line, cursor);

    if (cursor < line.size() && line[cursor] == ')') {
        ++cursor;

        return true;
    }

    while (cursor < line.size()) {
        if (destination.arguments.size() >= MaxArguments) {
            return false;
        }

        SimConfigFile::Argument argument;

        if (!ReadArgument(line, cursor, argument)) {
            return false;
        }

        destination.arguments.push_back(std::move(argument));
        SkipWhitespace(line, cursor);

        if (cursor >= line.size()) {
            return false;
        }

        if (line[cursor] == ')') {
            ++cursor;

            return true;
        }

        if (line[cursor] != ',') {
            return false;
        }

        ++cursor;

        SkipWhitespace(line, cursor);
    }

    return false;
}

[[nodiscard]] blitzar_status ParseLine(std::string_view line, SimConfigFile& destination)
{
    std::size_t cursor = 0;

    SkipWhitespace(line, cursor);

    if (cursor >= line.size() || line[cursor] == '#') {
        return BLITZAR_STATUS_OK;
    }

    SimConfigFile::Directive directive;

    if (!ReadDirective(line, cursor, directive)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    SkipWhitespace(line, cursor);

    if (cursor < line.size() && line[cursor] != '#') {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    destination.directives.push_back(std::move(directive));

    return BLITZAR_STATUS_OK;
}

} // namespace

blitzar_status ParseConfig(std::string_view source, SimConfigFile& destination) noexcept
{
    if (source.size() > MaxSourceBytes) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    SimConfigFile candidate;

    try {
        std::size_t line_start = 0;

        while (line_start <= source.size()) {
            const std::size_t line_end = source.find('\n', line_start);
            const std::size_t line_limit =
                line_end == std::string_view::npos ? source.size() : line_end;

            const blitzar_status status =
                ParseLine(source.substr(line_start, line_limit - line_start), candidate);

            if (status != BLITZAR_STATUS_OK) {
                return status;
            }

            if (candidate.directives.size() > MaxDirectives) {
                return BLITZAR_STATUS_INVALID_ARGUMENT;
            }

            if (line_end == std::string_view::npos) {
                break;
            }

            line_start = line_end + 1U;
        }

        destination = std::move(candidate);
    }
    catch (const std::length_error&) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }

    return BLITZAR_STATUS_OK;
}

blitzar_status LoadConfig(const std::filesystem::path& path, SimConfigFile& destination) noexcept
{
    std::error_code error;
    const std::uintmax_t file_size = std::filesystem::file_size(path, error);

    if (error || file_size > MaxSourceBytes) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    try {
        std::ifstream stream(path, std::ios::binary);

        if (!stream) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        std::string source;

        source.reserve(static_cast<std::size_t>(file_size));
        source.assign(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());

        if (stream.bad()) {
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }

        return ParseConfig(source, destination);
    }
    catch (const std::length_error&) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
}

} // namespace blitzar_sim
