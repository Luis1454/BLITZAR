#ifndef GRAVITY_SIM_TEXTPARSE_HPP
#define GRAVITY_SIM_TEXTPARSE_HPP

#include <algorithm>
#include <cctype>
#include <limits>
#include <string>
#include <string_view>
#include <type_traits>

namespace sim::text {

std::string_view trimView(std::string_view value);
bool parseSigned64(std::string_view rawValue, long long &out);
bool parseUnsigned64(std::string_view rawValue, unsigned long long &out);
bool parseFloat64(std::string_view rawValue, double &out);

template <typename NumberType>
inline bool parseNumber(std::string_view rawValue, NumberType &out)
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

} // namespace sim::text

#endif // GRAVITY_SIM_TEXTPARSE_HPP
