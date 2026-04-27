// File: engine/include/config/TextParse.hpp
// Purpose: Engine implementation for the BLITZAR simulation core.

#ifndef GRAVITY_ENGINE_INCLUDE_CONFIG_TEXTPARSE_HPP_
#define GRAVITY_ENGINE_INCLUDE_CONFIG_TEXTPARSE_HPP_
#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>
namespace grav_text {
/// Description: Executes the trimView operation.
std::string_view trimView(std::string_view value);
/// Description: Executes the parseSigned64 operation.
bool parseSigned64(std::string_view rawValue, long long& out);
/// Description: Executes the parseUnsigned64 operation.
bool parseUnsigned64(std::string_view rawValue, unsigned long long& out);
/// Description: Executes the parseFloat64 operation.
bool parseFloat64(std::string_view rawValue, double& out);
/// Description: Executes the parseNumber operation.
template <typename NumberType> bool parseNumber(std::string_view rawValue, NumberType& out);
extern template bool parseNumber<short>(std::string_view rawValue, short& out);
extern template bool parseNumber<unsigned short>(std::string_view rawValue, unsigned short& out);
extern template bool parseNumber<int>(std::string_view rawValue, int& out);
extern template bool parseNumber<unsigned int>(std::string_view rawValue, unsigned int& out);
extern template bool parseNumber<long>(std::string_view rawValue, long& out);
extern template bool parseNumber<unsigned long>(std::string_view rawValue, unsigned long& out);
extern template bool parseNumber<long long>(std::string_view rawValue, long long& out);
extern template bool parseNumber<unsigned long long>(std::string_view rawValue,
                                                     unsigned long long& out);
extern template bool parseNumber<float>(std::string_view rawValue, float& out);
extern template bool parseNumber<double>(std::string_view rawValue, double& out);
} // namespace grav_text
#endif // GRAVITY_ENGINE_INCLUDE_CONFIG_TEXTPARSE_HPP_
