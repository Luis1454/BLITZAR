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
    const char *rawValue = std::getenv(name);
    return rawValue != nullptr ? std::string(rawValue) : std::string();
}

void ScopedEnvVar::set(const char *value) const
{
    setenv(_name.c_str(), value, 1);
}

void ScopedEnvVar::clear() const
{
    unsetenv(_name.c_str());
}

} // namespace testsupport
