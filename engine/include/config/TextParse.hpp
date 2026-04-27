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
/// Description: Describes the parse number<short> operation contract.
extern template bool parseNumber<short>(std::string_view rawValue, short& out);
/// Description: Describes the short> operation contract.
extern template bool parseNumber<unsigned short>(std::string_view rawValue, unsigned short& out);
/// Description: Describes the parse number<int> operation contract.
extern template bool parseNumber<int>(std::string_view rawValue, int& out);
/// Description: Describes the int> operation contract.
extern template bool parseNumber<unsigned int>(std::string_view rawValue, unsigned int& out);
/// Description: Describes the parse number<long> operation contract.
extern template bool parseNumber<long>(std::string_view rawValue, long& out);
/// Description: Describes the long> operation contract.
extern template bool parseNumber<unsigned long>(std::string_view rawValue, unsigned long& out);
/// Description: Describes the long> operation contract.
extern template bool parseNumber<long long>(std::string_view rawValue, long long& out);
/// Description: Describes the long> operation contract.
extern template bool parseNumber<unsigned long long>(std::string_view rawValue,
                                                     unsigned long long& out);
/// Description: Describes the parse number<float> operation contract.
extern template bool parseNumber<float>(std::string_view rawValue, float& out);
/// Description: Describes the parse number<double> operation contract.
extern template bool parseNumber<double>(std::string_view rawValue, double& out);
} // namespace grav_text
#endif // GRAVITY_ENGINE_INCLUDE_CONFIG_TEXTPARSE_HPP_
