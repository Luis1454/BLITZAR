// File: tests/support/scoped_env_var_win.cpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#include "tests/support/scoped_env_var.hpp"
#include <cstdlib>
namespace testsupport {
ScopedEnvVar::ScopedEnvVar(const char* name, const char* value) : _name(name), _hadValue(false)
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
std::string ScopedEnvVar::read(const char* name)
{
    char* rawValue = nullptr;
    std::size_t rawSize = 0;
    if (_dupenv_s(&rawValue, &rawSize, name) != 0 || rawValue == nullptr) {
        return {};
    }
    const std::string value(rawValue);
    std::free(rawValue);
    return value;
}
void ScopedEnvVar::set(const char* value) const
{
    _putenv_s(_name.c_str(), value);
}
void ScopedEnvVar::clear() const
{
    _putenv_s(_name.c_str(), "");
}
} // namespace testsupport
