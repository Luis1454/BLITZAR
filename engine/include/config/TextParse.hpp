#ifndef GRAVITY_ENGINE_INCLUDE_CONFIG_TEXTPARSE_HPP_
#define GRAVITY_ENGINE_INCLUDE_CONFIG_TEXTPARSE_HPP_

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>

namespace grav_text {

std::string_view trimView(std::string_view value);
bool parseSigned64(std::string_view rawValue, long long &out);
bool parseUnsigned64(std::string_view rawValue, unsigned long long &out);
bool parseFloat64(std::string_view rawValue, double &out);

template <typename NumberType>
bool parseNumber(std::string_view rawValue, NumberType &out);

extern template bool parseNumber<short>(std::string_view rawValue, short &out);
extern template bool parseNumber<unsigned short>(std::string_view rawValue, unsigned short &out);
extern template bool parseNumber<int>(std::string_view rawValue, int &out);
extern template bool parseNumber<unsigned int>(std::string_view rawValue, unsigned int &out);
extern template bool parseNumber<long>(std::string_view rawValue, long &out);
extern template bool parseNumber<unsigned long>(std::string_view rawValue, unsigned long &out);
extern template bool parseNumber<long long>(std::string_view rawValue, long long &out);
extern template bool parseNumber<unsigned long long>(std::string_view rawValue, unsigned long long &out);
extern template bool parseNumber<float>(std::string_view rawValue, float &out);
extern template bool parseNumber<double>(std::string_view rawValue, double &out);

} // namespace grav_text


#endif // GRAVITY_ENGINE_INCLUDE_CONFIG_TEXTPARSE_HPP_
