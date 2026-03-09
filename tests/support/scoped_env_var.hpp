#pragma once

#include <string>

namespace testsupport {

class ScopedEnvVar {
    public:
        ScopedEnvVar(const char *name, const char *value);
        ~ScopedEnvVar();

        ScopedEnvVar(const ScopedEnvVar &) = delete;
        ScopedEnvVar &operator=(const ScopedEnvVar &) = delete;

    private:
        static std::string read(const char *name);
        void set(const char *value) const;
        void clear() const;

        std::string _name;
        std::string _previousValue;
        bool _hadValue;
};

} // namespace testsupport

