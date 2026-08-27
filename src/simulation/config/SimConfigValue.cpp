#include "simulation/config/SimConfigValue.hpp"

#include <charconv>
#include <cmath>
#include <locale>
#include <new>
#include <sstream>
#include <string>
#include <system_error>

namespace blitzar_sim {

namespace {

template <typename Integer>
[[nodiscard]] bool ParseInteger(std::string_view text, Integer& value) noexcept
{
    Integer parsed{};
    const auto result = std::from_chars(text.data(), text.data() + text.size(), parsed);

    if (result.ec != std::errc{} || result.ptr != text.data() + text.size()) {
        return false;
    }

    value = parsed;

    return true;
}

[[nodiscard]] bool ParseReal(std::string_view text, double& value) noexcept
{
    try {
        std::istringstream input{std::string(text)};

        input.imbue(std::locale::classic());

        input >> std::noskipws;

        double parsed{};

        input >> parsed;

        if (input.fail() || !std::isfinite(parsed) ||
            input.peek() != std::char_traits<char>::eof()) {
            return false;
        }

        value = parsed;

        return true;
    }
    catch (const std::bad_alloc&) {
        return false;
    }
}

[[nodiscard]] bool ReadNamedValue(const SimConfigFile::Directive& directive, std::string_view name,
    std::string_view& value) noexcept
{
    std::size_t matches = 0;

    for (const SimConfigFile::Argument& argument : directive.arguments) {
        if (argument.name == name) {
            value = argument.value.text;

            ++matches;
        }
    }

    return matches == 1U;
}

[[nodiscard]] bool ReadQuotedNamedValue(const SimConfigFile::Directive& directive,
    std::string_view name, std::string_view& value) noexcept
{
    std::size_t matches = 0;

    for (const SimConfigFile::Argument& argument : directive.arguments) {
        if (argument.name == name && argument.value.quoted) {
            value = argument.value.text;

            ++matches;
        }
    }

    return matches == 1U;
}

} // namespace

bool HasExactArguments(
    const SimConfigFile::Directive& directive, std::span<const std::string_view> expected) noexcept
{
    if (directive.arguments.size() != expected.size()) {
        return false;
    }

    for (const std::string_view name : expected) {
        std::size_t matches = 0;

        for (const SimConfigFile::Argument& argument : directive.arguments) {
            if (argument.name == name) {
                ++matches;
            }
        }

        if (matches != 1U) {
            return false;
        }
    }

    return true;
}

bool ReadConfigText(const SimConfigFile::Directive& directive, std::string_view name,
    std::string_view& value) noexcept
{
    return ReadNamedValue(directive, name, value);
}

bool ReadConfigQuotedText(const SimConfigFile::Directive& directive, std::string_view name,
    std::string_view& value) noexcept
{
    return ReadQuotedNamedValue(directive, name, value);
}

bool ReadConfigInteger(
    const SimConfigFile::Directive& directive, std::string_view name, std::int64_t& value) noexcept
{
    std::string_view text;

    return ReadNamedValue(directive, name, text) && ParseInteger(text, value);
}

bool ReadConfigUnsigned(
    const SimConfigFile::Directive& directive, std::string_view name, std::uint64_t& value) noexcept
{
    std::string_view text;

    return ReadNamedValue(directive, name, text) && ParseInteger(text, value);
}

bool ReadConfigReal(
    const SimConfigFile::Directive& directive, std::string_view name, double& value) noexcept
{
    std::string_view text;

    return ReadNamedValue(directive, name, text) && ParseReal(text, value);
}

bool ReadConfigBoolean(
    const SimConfigFile::Directive& directive, std::string_view name, bool& value) noexcept
{
    std::string_view text;

    if (!ReadNamedValue(directive, name, text)) {
        return false;
    }

    if (text == "true") {
        value = true;

        return true;
    }

    if (text == "false") {
        value = false;

        return true;
    }

    return false;
}

} // namespace blitzar_sim
