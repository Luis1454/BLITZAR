#include "config/EnvUtilsWin.hpp"
#include "config/EnvUtils.hpp"
namespace grav_env {
std::optional<std::string> get(std::string_view name)
{
    const std::string key(name);
    char* rawValue = nullptr;
    std::size_t rawSize = 0;
    if (_dupenv_s(&rawValue, &rawSize, key.c_str()) != 0 || rawValue == nullptr) {
        return std::nullopt;
    }
    const std::string value(rawValue);
    std::free(rawValue);
    return value;
}
} // namespace grav_env
