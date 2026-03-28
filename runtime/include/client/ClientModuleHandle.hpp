#ifndef GRAVITY_RUNTIME_INCLUDE_CLIENT_CLIENTMODULEHANDLE_HPP_
#define GRAVITY_RUNTIME_INCLUDE_CLIENT_CLIENTMODULEHANDLE_HPP_
#include <memory>
#include <string>
#include <string_view>
namespace grav_module {
class ClientModuleHandle {
public:
    ClientModuleHandle();
    ~ClientModuleHandle();
    ClientModuleHandle(ClientModuleHandle&& other) noexcept;
    ClientModuleHandle& operator=(ClientModuleHandle&& other) noexcept;
    ClientModuleHandle(const ClientModuleHandle&) = delete;
    ClientModuleHandle& operator=(const ClientModuleHandle&) = delete;
    bool load(const std::string& modulePath, const std::string& configPath,
              std::string_view expectedModuleId, std::string& outError);
    void unload() noexcept;
    [[nodiscard]] bool isLoaded() const noexcept;
    [[nodiscard]] std::string_view moduleName() const noexcept;
    [[nodiscard]] std::string_view loadedPath() const noexcept;
    bool handleCommand(std::string_view commandLine, bool& outKeepRunning, std::string& outError);

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};
} // namespace grav_module
#endif // GRAVITY_RUNTIME_INCLUDE_CLIENT_CLIENTMODULEHANDLE_HPP_
