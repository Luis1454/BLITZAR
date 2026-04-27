// File: engine/include/config/DirectiveValueFormatter.hpp
// Purpose: Engine implementation for the BLITZAR simulation core.

#ifndef GRAVITY_ENGINE_INCLUDE_CONFIG_DIRECTIVEVALUEFORMATTER_HPP_
#define GRAVITY_ENGINE_INCLUDE_CONFIG_DIRECTIVEVALUEFORMATTER_HPP_
#include <string>
namespace grav_config {
/// Description: Defines the DirectiveValueFormatter data or behavior contract.
class DirectiveValueFormatter final {
public:
    [[nodiscard]] static std::string quote(const std::string& value);
};
} // namespace grav_config
#endif // GRAVITY_ENGINE_INCLUDE_CONFIG_DIRECTIVEVALUEFORMATTER_HPP_
