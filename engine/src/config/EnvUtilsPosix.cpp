// File: engine/src/config/EnvUtilsPosix.cpp
// Purpose: Engine implementation for the BLITZAR simulation core.

#include "config/EnvUtilsPosix.hpp"
#include "config/EnvUtils.hpp"
namespace grav_env {
/// Description: Executes the get operation.
std::optional<std::string> get(std::string_view name)
{
    /// Description: Executes the key operation.
    const std::string key(name);
    const char* rawValue = std::getenv(key.c_str());
    if (rawValue == nullptr)
        return std::nullopt;
    return std::string(rawValue);
}
} // namespace grav_env
