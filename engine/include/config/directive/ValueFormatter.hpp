/*
 * @file engine/include/config/directive/ValueFormatter.hpp
 * @author BLITZAR Contributors
 * @project BLITZAR
 * @brief Public configuration interfaces and validation contracts for simulation setup.
 */

#ifndef BLITZAR_ENGINE_INCLUDE_CONFIG_DIRECTIVEVALUEFORMATTER_HPP_
#define BLITZAR_ENGINE_INCLUDE_CONFIG_DIRECTIVEVALUEFORMATTER_HPP_
#include <string>

namespace bltzr_config {
[[nodiscard]] std::string quoteDirectiveValue(const std::string& value);
} // namespace bltzr_config
#endif // BLITZAR_ENGINE_INCLUDE_CONFIG_DIRECTIVEVALUEFORMATTER_HPP_
