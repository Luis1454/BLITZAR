#ifndef GRAVITY_RUNTIME_INCLUDE_CLIENT_CLIENTMODULEMANIFEST_HPP_
#define GRAVITY_RUNTIME_INCLUDE_CLIENT_CLIENTMODULEMANIFEST_HPP_
#include <cstdint>
#include <string>
#include <string_view>
namespace grav_module {
class ClientModuleManifest final {
public:
    static bool load(std::string_view modulePath, ClientModuleManifest& outManifest,
                     std::string& outError);
    bool validateForLoad(std::string_view modulePath, std::string_view expectedModuleId,
                         std::string& outError) const;
    [[nodiscard]] std::string_view moduleId() const noexcept;
    [[nodiscard]] std::string_view moduleName() const noexcept;
    [[nodiscard]] std::string_view sha256() const noexcept;
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
