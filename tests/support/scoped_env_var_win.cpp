// File: tests/support/scoped_env_var_win.cpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#include "tests/support/scoped_env_var.hpp"
#include <cstdlib>
namespace testsupport {
/// Description: Executes the ScopedEnvVar operation.
ScopedEnvVar::ScopedEnvVar(const char* name, const char* value) : _name(name), _hadValue(false)
{
    const std::string current = read(name);
    if (!current.empty()) {
        _hadValue = true;
        _previousValue = current;
    }
    /// Description: Executes the set operation.
    set(value);
}
/// Description: Releases resources owned by ScopedEnvVar.
ScopedEnvVar::~ScopedEnvVar()
{
    if (_hadValue) {
        /// Description: Executes the set operation.
        set(_previousValue.c_str());
        return;
    }
    /// Description: Executes the clear operation.
    clear();
}
/// Description: Executes the read operation.
std::string ScopedEnvVar::read(const char* name)
{
    char* rawValue = nullptr;
    std::size_t rawSize = 0;
    if (_dupenv_s(&rawValue, &rawSize, name) != 0 || rawValue == nullptr) {
        return {};
    }
    /// Description: Executes the value operation.
    const std::string value(rawValue);
    /// Description: Executes the free operation.
    std::free(rawValue);
    return value;
}
/// Description: Executes the set operation.
void ScopedEnvVar::set(const char* value) const
{
    /// Description: Executes the _putenv_s operation.
    _putenv_s(_name.c_str(), value);
}
/// Description: Executes the clear operation.
void ScopedEnvVar::clear() const
{
    /// Description: Executes the _putenv_s operation.
    _putenv_s(_name.c_str(), "");
}
} // namespace testsupport
