// File: runtime/include/client/ClientModuleManifest.hpp
// Purpose: Runtime integration surface for BLITZAR clients and protocols.

#ifndef GRAVITY_RUNTIME_INCLUDE_CLIENT_CLIENTMODULEMANIFEST_HPP_
#define GRAVITY_RUNTIME_INCLUDE_CLIENT_CLIENTMODULEMANIFEST_HPP_
#include <cstdint>
#include <string>
#include <string_view>

namespace grav_module {
/// Description: Defines the ClientModuleManifest data or behavior contract.
class ClientModuleManifest final {
public:
    /// Description: Describes the load operation contract.
    static bool load(std::string_view modulePath, ClientModuleManifest& outManifest,
                     std::string& outError);
    /// Description: Describes the validate for load operation contract.
    bool validateForLoad(std::string_view modulePath, std::string_view expectedModuleId,
                         std::string& outError) const;
    /// Description: Describes the module id operation contract.
    [[nodiscard]] std::string_view moduleId() const noexcept;
    /// Description: Describes the module name operation contract.
    [[nodiscard]] std::string_view moduleName() const noexcept;
    /// Description: Describes the sha256 operation contract.
    [[nodiscard]] std::string_view sha256() const noexcept;
    /// Description: Describes the api version operation contract.
    [[nodiscard]] std::uint32_t apiVersion() const noexcept;

private:
    std::string m_moduleId;
    std::string m_moduleName;
    std::string m_productName;
    std::string m_productVersion;
    std::string m_libraryFile;
    std::string m_sha256;
    std::uint32_t m_formatVersion = 0u;
    std::uint32_t m_apiVersion = 0u;
};
} // namespace grav_module
#endif // GRAVITY_RUNTIME_INCLUDE_CLIENT_CLIENTMODULEMANIFEST_HPP_
