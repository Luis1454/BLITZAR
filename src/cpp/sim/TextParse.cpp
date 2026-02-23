#include "sim/TextParse.hpp"

#include <charconv>
#include <limits>
#include <locale>
#include <sstream>
#include <string>
#include <system_error>

namespace sim::text {

std::string_view trimView(std::string_view value)
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
    const std::size_t beginOffset = static_cast<std::size_t>(std::distance(value.begin(), begin));
    const std::size_t trimmedSize = static_cast<std::size_t>(std::distance(begin, end));
    return value.substr(beginOffset, trimmedSize);
}

bool parseSigned64(std::string_view rawValue, long long &out)
{
    const std::string_view trimmed = trimView(rawValue);
    if (trimmed.empty()) {
        return false;
    }
    long long parsed = 0;
    const auto [ptr, ec] = std::from_chars(trimmed.data(), trimmed.data() + trimmed.size(), parsed, 10);
    if (ec != std::errc{} || ptr != (trimmed.data() + trimmed.size())) {
        return false;
    }
    out = parsed;
    return true;
}

bool parseUnsigned64(std::string_view rawValue, unsigned long long &out)
{
    const std::string_view trimmed = trimView(rawValue);
    if (trimmed.empty()) {
        return false;
    }
    unsigned long long parsed = 0;
    const auto [ptr, ec] = std::from_chars(trimmed.data(), trimmed.data() + trimmed.size(), parsed, 10);
    if (ec != std::errc{} || ptr != (trimmed.data() + trimmed.size())) {
        return false;
    }
    out = parsed;
    return true;
}

bool parseFloat64(std::string_view rawValue, double &out)
{
    const std::string_view trimmed = trimView(rawValue);
    if (trimmed.empty()) {
        return false;
    }

#if defined(__cpp_lib_to_chars) && (__cpp_lib_to_chars >= 201611L)
    double parsed = 0.0;
    const auto [ptr, ec] = std::from_chars(
        trimmed.data(),
        trimmed.data() + trimmed.size(),
        parsed,
        std::chars_format::general);
    if (ec == std::errc{} && ptr == (trimmed.data() + trimmed.size())) {
        out = parsed;
        return true;
    }
#endif

    std::istringstream input{std::string(trimmed)};
    input.imbue(std::locale::classic());
    long double parsedFallback = 0.0L;
    input >> parsedFallback;
    if (!input) {
        return false;
    }
    input >> std::ws;
    if (!input.eof()) {
        return false;
    }
    if (parsedFallback < -std::numeric_limits<double>::max()
        || parsedFallback > std::numeric_limits<double>::max()) {
        return false;
    }
    out = static_cast<double>(parsedFallback);
    return true;
}

} // namespace sim::text
