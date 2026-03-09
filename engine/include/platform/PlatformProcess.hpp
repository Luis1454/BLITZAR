#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace grav_platform {

    std::string quoteProcessArg(const std::string &arg);
    std::string buildProcessCommandLine(const std::string &executable, const std::vector<std::string> &args);

    class ProcessHandle {
        public:
            ProcessHandle();
            ~ProcessHandle();

            ProcessHandle(const ProcessHandle &) = delete;
            ProcessHandle &operator=(const ProcessHandle &) = delete;

            ProcessHandle(ProcessHandle &&other) noexcept;
            ProcessHandle &operator=(ProcessHandle &&other) noexcept;

            bool launch(const std::string &executable, const std::vector<std::string> &args,
                        bool createNewConsole, std::string &outError);

            bool terminate(std::uint32_t waitMs, std::string &outError);
            bool isRunning() const;
            void clear();
            std::string pidString() const;
            const std::string &commandLine() const;

        private:
            struct Impl;
            std::unique_ptr<Impl> _impl;
    };

    bool launchDetachedProcess(const std::string &executable, const std::vector<std::string> &args, std::string &outError);
    int runProcessBlocking(const std::string &executable, const std::vector<std::string> &args,
                            bool createNewConsole, std::string &outError);

} // namespace grav_platform

