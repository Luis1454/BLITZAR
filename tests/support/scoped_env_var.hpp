// File: tests/support/scoped_env_var.hpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#ifndef GRAVITY_TESTS_SUPPORT_SCOPED_ENV_VAR_HPP_
#define GRAVITY_TESTS_SUPPORT_SCOPED_ENV_VAR_HPP_
#include <string>

namespace testsupport {
/// Description: Defines the ScopedEnvVar data or behavior contract.
class ScopedEnvVar {
public:
    /// Description: Describes the scoped env var operation contract.
    ScopedEnvVar(const char* name, const char* value);
    /// Description: Releases resources owned by ScopedEnvVar.
    ~ScopedEnvVar();
    /// Description: Describes the scoped env var operation contract.
    ScopedEnvVar(const ScopedEnvVar&) = delete;
    /// Description: Describes the operator= operation contract.
    ScopedEnvVar& operator=(const ScopedEnvVar&) = delete;

private:
    /// Description: Describes the read operation contract.
    static std::string read(const char* name);
    /// Description: Describes the set operation contract.
    void set(const char* value) const;
    /// Description: Describes the clear operation contract.
    void clear() const;
    std::string _name;
    std::string _previousValue;
    bool _hadValue;
};
} // namespace testsupport
#endif // GRAVITY_TESTS_SUPPORT_SCOPED_ENV_VAR_HPP_
