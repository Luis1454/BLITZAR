#ifndef GRAVITY_ENGINE_INCLUDE_CONFIG_DIRECTIVEVALUEFORMATTER_HPP_
#define GRAVITY_ENGINE_INCLUDE_CONFIG_DIRECTIVEVALUEFORMATTER_HPP_

#include <string>

namespace grav_config {

class DirectiveValueFormatter final {
public:
    [[nodiscard]] static std::string quote(const std::string &value);
};

} // namespace grav_config

#endif // GRAVITY_ENGINE_INCLUDE_CONFIG_DIRECTIVEVALUEFORMATTER_HPP_
