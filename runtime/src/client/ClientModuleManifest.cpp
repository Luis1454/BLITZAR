// File: runtime/src/client/ClientModuleManifest.cpp
// Purpose: Runtime integration surface for BLITZAR clients and protocols.

#include "client/ClientModuleManifest.hpp"
#include "client/ClientModuleApi.hpp"
#include "config/TextParse.hpp"
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
namespace grav_module {
/// Description: Defines the ClientModuleManifestLocal data or behavior contract.
class ClientModuleManifestLocal final {
public:
    static bool isSupportedModuleId(std::string_view moduleId) noexcept
    {
        return moduleId == "cli" || moduleId == "echo" || moduleId == "gui" || moduleId == "qt";
    }
    /// Description: Executes the parseLine operation.
    static bool parseLine(std::string_view line, std::string& outKey, std::string& outValue)
    {
        const std::string_view trimmed = grav_text::trimView(line);
        if (trimmed.empty() || trimmed.front() == '#') {
            return false;
        }
        const std::size_t separator = trimmed.find('=');
        if (separator == std::string_view::npos)
            return false;
        outKey.assign(grav_text::trimView(trimmed.substr(0u, separator)));
        outValue.assign(grav_text::trimView(trimmed.substr(separator + 1u)));
        return !outKey.empty();
    }
    /// Description: Executes the readUnsigned operation.
    static bool readUnsigned(std::string_view rawValue, std::uint32_t& outValue)
    {
        unsigned int parsed = 0u;
        if (!grav_text::parseNumber(rawValue, parsed)) {
            return false;
        }
        outValue = parsed;
        return true;
    }
    static bool isHexDigest(std::string_view rawValue) noexcept
    {
        if (rawValue.size() != 64u) {
            return false;
        }
        for (const char ch : rawValue) {
            const bool digit = ch >= '0' && ch <= '9';
            const bool lowerHex = ch >= 'a' && ch <= 'f';
            if (!digit && !lowerHex) {
                return false;
            }
        }
        return true;
    }
};
bool ClientModuleManifest::load(std::string_view modulePath, ClientModuleManifest& outManifest,
                                std::string& outError)
{
    outManifest = ClientModuleManifest{};
    const std::filesystem::path moduleFile{std::string(modulePath)};
    const std::filesystem::path manifestPath{moduleFile.string() + ".manifest"};
    /// Description: Executes the input operation.
    std::ifstream input(manifestPath, std::ios::binary);
    if (!input.is_open()) {
        outError = "module manifest missing: " + manifestPath.string();
        return false;
    }
    std::string line;
    while (std::getline(input, line)) {
        std::string key;
        std::string value;
        if (!ClientModuleManifestLocal::parseLine(line, key, value)) {
            continue;
        }
        if (key == "format_version") {
            if (!ClientModuleManifestLocal::readUnsigned(value, outManifest.m_formatVersion)) {
                outError = "invalid module manifest format_version";
                return false;
            }
            continue;
        }
        if (key == "module_id") {
            outManifest.m_moduleId = value;
            continue;
        }
        if (key == "module_name") {
            outManifest.m_moduleName = value;
            continue;
        }
        if (key == "product_name") {
            outManifest.m_productName = value;
            continue;
        }
        if (key == "product_version") {
            outManifest.m_productVersion = value;
            continue;
        }
        if (key == "library_file") {
            outManifest.m_libraryFile = value;
            continue;
        }
        if (key == "sha256") {
            outManifest.m_sha256 = value;
            continue;
        }
        if (key == "api_version") {
            if (!ClientModuleManifestLocal::readUnsigned(value, outManifest.m_apiVersion)) {
                outError = "invalid module manifest api_version";
                return false;
            }
        }
    }
    if (outManifest.m_formatVersion != 1u || outManifest.m_moduleId.empty() ||
        outManifest.m_moduleName.empty() || outManifest.m_productName.empty() ||
        outManifest.m_productVersion.empty() || outManifest.m_libraryFile.empty() ||
        outManifest.m_sha256.empty()) {
        outError = "module manifest is incomplete";
        return false;
    }
    if (!ClientModuleManifestLocal::isHexDigest(outManifest.m_sha256)) {
        outError = "module manifest sha256 is invalid";
        return false;
    }
    outError.clear();
    return true;
}
bool ClientModuleManifest::validateForLoad(std::string_view modulePath,
                                           std::string_view expectedModuleId,
                                           std::string& outError) const
{
    if (!ClientModuleManifestLocal::isSupportedModuleId(m_moduleId)) {
        outError = "unsupported module id in manifest: " + m_moduleId;
        return false;
    }
    if (!expectedModuleId.empty() && expectedModuleId != m_moduleId) {
        outError = "module manifest id mismatch";
        return false;
    }
    if (m_apiVersion != kClientModuleApiVersionV1) {
        outError = "unsupported module api version";
        return false;
    }
    if (m_productName != kClientModuleProductName) {
        outError = "module manifest product mismatch";
        return false;
    }
    if (m_productVersion != kClientModuleProductVersion) {
        outError = "module manifest product version mismatch";
        return false;
    }
    const std::filesystem::path moduleFile{std::string(modulePath)};
    if (moduleFile.filename().string() != m_libraryFile) {
        outError = "module manifest library_file mismatch";
        return false;
    }
    outError.clear();
    return true;
}
std::string_view ClientModuleManifest::moduleId() const noexcept
{
    return m_moduleId;
}
std::string_view ClientModuleManifest::moduleName() const noexcept
{
    return m_moduleName;
}
std::string_view ClientModuleManifest::sha256() const noexcept
{
    return m_sha256;
}
std::uint32_t ClientModuleManifest::apiVersion() const noexcept
{
    return m_apiVersion;
}
} // namespace grav_module
