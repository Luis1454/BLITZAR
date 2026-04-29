/*
 * @file engine/include/config/DirectiveValueFormatter.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Public configuration interfaces and validation contracts for simulation setup.
 */

#ifndef BLITZAR_ENGINE_INCLUDE_CONFIG_DIRECTIVEVALUEFORMATTER_HPP_
#define BLITZAR_ENGINE_INCLUDE_CONFIG_DIRECTIVEVALUEFORMATTER_HPP_
#include <string>

namespace bltzr_config {
class DirectiveValueFormatter final {
public:
    [[nodiscard]] static std::string quote(const std::string& value);
};
} // namespace bltzr_config
#endif // BLITZAR_ENGINE_INCLUDE_CONFIG_DIRECTIVEVALUEFORMATTER_HPP_
