// File: tests/support/scoped_env_var_posix.cpp
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
    const char* rawValue = std::getenv(name);
    return rawValue != nullptr ? std::string(rawValue) : std::string();
}
/// Description: Executes the set operation.
void ScopedEnvVar::set(const char* value) const
{
    /// Description: Executes the setenv operation.
    setenv(_name.c_str(), value, 1);
}
/// Description: Executes the clear operation.
void ScopedEnvVar::clear() const
{
    /// Description: Executes the unsetenv operation.
    unsetenv(_name.c_str());
}
} // namespace testsupport
