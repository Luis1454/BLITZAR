/*
 * @file engine/src/config/EnvUtilsPosix.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Configuration parsing, validation, and serialization implementation.
 */

#include "config/EnvUtilsPosix.hpp"
#include "config/EnvUtils.hpp"

namespace bltzr_env {
std::optional<std::string> get(std::string_view name)
{
    const std::string key(name);
    const char* rawValue = std::getenv(key.c_str());
    if (rawValue == nullptr)
        return std::nullopt;
    return std::string(rawValue);
}
} // namespace bltzr_env
