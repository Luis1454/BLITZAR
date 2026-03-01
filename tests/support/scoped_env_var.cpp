#include "tests/support/scoped_env_var.hpp"

#include <cstdlib>

namespace testsupport {

ScopedEnvVar::ScopedEnvVar(const char *name, const char *value)
    : _name(name), _hadValue(false)
{
    const std::string current = read(name);
    if (!current.empty()) {
        _hadValue = true;
        _previousValue = current;
    }
    set(value);
}

ScopedEnvVar::~ScopedEnvVar()
{
    if (_hadValue) {
        set(_previousValue.c_str());
        return;
    }
    clear();
}

std::string ScopedEnvVar::read(const char *name)
{
#if defined(_WIN32)
    char *rawValue = nullptr;
    std::size_t rawSize = 0;
    if (_dupenv_s(&rawValue, &rawSize, name) != 0 || rawValue == nullptr) {
        return {};
    }
    std::string value(rawValue);
    std::free(rawValue);
    return value;
#else
    const char *rawValue = std::getenv(name);
    return rawValue != nullptr ? std::string(rawValue) : std::string();
#endif
}

void ScopedEnvVar::set(const char *value) const
{
#if defined(_WIN32)
    _putenv_s(_name.c_str(), value);
#else
    setenv(_name.c_str(), value, 1);
#endif
}

void ScopedEnvVar::clear() const
{
#if defined(_WIN32)
    _putenv_s(_name.c_str(), "");
#else
    unsetenv(_name.c_str());
#endif
}

} // namespace testsupport
