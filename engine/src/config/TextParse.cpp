#include "config/TextParse.hpp"

#include <charconv>
#include <limits>
#include <locale>
#include <sstream>
#include <string>
#include <system_error>
#include <type_traits>

namespace grav_text {

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

template <typename NumberType>
bool parseNumber(std::string_view rawValue, NumberType &out)
{
    if constexpr (std::is_integral_v<NumberType>) {
        if constexpr (std::is_signed_v<NumberType>) {
            long long parsed = 0;
            if (!parseSigned64(rawValue, parsed)) {
                return false;
            }
            if (parsed < static_cast<long long>(std::numeric_limits<NumberType>::lowest())
                || parsed > static_cast<long long>(std::numeric_limits<NumberType>::max())) {
                return false;
            }
            out = static_cast<NumberType>(parsed);
            return true;
        } else {
            unsigned long long parsed = 0;
            if (!parseUnsigned64(rawValue, parsed)) {
                return false;
            }
            if (parsed > static_cast<unsigned long long>(std::numeric_limits<NumberType>::max())) {
                return false;
            }
            out = static_cast<NumberType>(parsed);
            return true;
        }
    } else if constexpr (std::is_floating_point_v<NumberType>) {
        double parsed = 0.0;
        if (!parseFloat64(rawValue, parsed)) {
            return false;
        }
        if (parsed < static_cast<double>(std::numeric_limits<NumberType>::lowest())
            || parsed > static_cast<double>(std::numeric_limits<NumberType>::max())) {
            return false;
        }
        out = static_cast<NumberType>(parsed);
        return true;
    } else {
        static_assert(std::is_arithmetic_v<NumberType>, "parseNumber requires arithmetic type");
        return false;
    }
}

template bool parseNumber<short>(std::string_view rawValue, short &out);
template bool parseNumber<unsigned short>(std::string_view rawValue, unsigned short &out);
template bool parseNumber<int>(std::string_view rawValue, int &out);
template bool parseNumber<unsigned int>(std::string_view rawValue, unsigned int &out);
template bool parseNumber<long>(std::string_view rawValue, long &out);
template bool parseNumber<unsigned long>(std::string_view rawValue, unsigned long &out);
template bool parseNumber<long long>(std::string_view rawValue, long long &out);
template bool parseNumber<unsigned long long>(std::string_view rawValue, unsigned long long &out);
template bool parseNumber<float>(std::string_view rawValue, float &out);
template bool parseNumber<double>(std::string_view rawValue, double &out);

} // namespace grav_text

