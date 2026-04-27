// File: runtime/include/client/ClientModuleHandle.hpp
// Purpose: Runtime integration surface for BLITZAR clients and protocols.

#ifndef GRAVITY_RUNTIME_INCLUDE_CLIENT_CLIENTMODULEHANDLE_HPP_
#define GRAVITY_RUNTIME_INCLUDE_CLIENT_CLIENTMODULEHANDLE_HPP_
#include <memory>
#include <string>
#include <string_view>

namespace grav_module {
/// Description: Defines the ClientModuleHandle data or behavior contract.
class ClientModuleHandle {
public:
    /// Description: Describes the client module handle operation contract.
    ClientModuleHandle();
    /// Description: Releases resources owned by ClientModuleHandle.
    ~ClientModuleHandle();
    /// Description: Describes the client module handle operation contract.
    ClientModuleHandle(ClientModuleHandle&& other) noexcept;
    /// Description: Describes the operator= operation contract.
    ClientModuleHandle& operator=(ClientModuleHandle&& other) noexcept;
    /// Description: Describes the client module handle operation contract.
    ClientModuleHandle(const ClientModuleHandle&) = delete;
    /// Description: Describes the operator= operation contract.
    ClientModuleHandle& operator=(const ClientModuleHandle&) = delete;
    /// Description: Describes the load operation contract.
    bool load(const std::string& modulePath, const std::string& configPath,
              std::string_view expectedModuleId, std::string& outError);
    /// Description: Describes the unload operation contract.
    void unload() noexcept;
    /// Description: Describes the is loaded operation contract.
    [[nodiscard]] bool isLoaded() const noexcept;
    /// Description: Describes the module name operation contract.
    [[nodiscard]] std::string_view moduleName() const noexcept;
    /// Description: Describes the loaded path operation contract.
    [[nodiscard]] std::string_view loadedPath() const noexcept;
    /// Description: Describes the handle command operation contract.
    bool handleCommand(std::string_view commandLine, bool& outKeepRunning, std::string& outError);

private:
    /// Description: Defines the Impl data or behavior contract.
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};
} // namespace grav_module
#endif // GRAVITY_RUNTIME_INCLUDE_CLIENT_CLIENTMODULEHANDLE_HPP_
