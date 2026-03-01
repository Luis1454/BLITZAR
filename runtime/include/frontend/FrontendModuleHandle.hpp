#pragma once

#include <memory>
#include <string>
#include <string_view>

namespace grav_module {

class FrontendModuleHandle {
public:
    FrontendModuleHandle();
    ~FrontendModuleHandle();

    FrontendModuleHandle(FrontendModuleHandle &&other) noexcept;
    FrontendModuleHandle &operator=(FrontendModuleHandle &&other) noexcept;

    FrontendModuleHandle(const FrontendModuleHandle &) = delete;
    FrontendModuleHandle &operator=(const FrontendModuleHandle &) = delete;

    bool load(const std::string &modulePath, const std::string &configPath, std::string &outError);
    void unload() noexcept;

    [[nodiscard]] bool isLoaded() const noexcept;
    [[nodiscard]] std::string_view moduleName() const noexcept;
    [[nodiscard]] std::string_view loadedPath() const noexcept;

    bool handleCommand(std::string_view commandLine, bool &outKeepRunning, std::string &outError);

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace grav_module
