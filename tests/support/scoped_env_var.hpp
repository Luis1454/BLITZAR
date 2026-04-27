// File: tests/support/scoped_env_var.hpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#ifndef GRAVITY_TESTS_SUPPORT_SCOPED_ENV_VAR_HPP_
#define GRAVITY_TESTS_SUPPORT_SCOPED_ENV_VAR_HPP_
#include <string>
namespace testsupport {
/// Description: Defines the ScopedEnvVar data or behavior contract.
class ScopedEnvVar {
public:
    /// Description: Executes the ScopedEnvVar operation.
    ScopedEnvVar(const char* name, const char* value);
    /// Description: Releases resources owned by ScopedEnvVar.
    ~ScopedEnvVar();
    ScopedEnvVar(const ScopedEnvVar&) = delete;
    ScopedEnvVar& operator=(const ScopedEnvVar&) = delete;

private:
    /// Description: Executes the read operation.
    static std::string read(const char* name);
    /// Description: Executes the set operation.
    void set(const char* value) const;
    /// Description: Executes the clear operation.
    void clear() const;
    std::string _name;
    std::string _previousValue;
    bool _hadValue;
};
} // namespace testsupport
#endif // GRAVITY_TESTS_SUPPORT_SCOPED_ENV_VAR_HPP_
