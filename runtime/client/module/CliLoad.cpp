/*
 * @file runtime/client/module/CliLoad.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Runtime implementation for protocol, command, client, and FFI boundaries.
 */

#include "CliLoad.hpp"
#include "client/module/CliHandle.hpp"
#include "client/module/CliHash.hpp"
#include "client/module/CliManifest.hpp"
#include "Internal.hpp"
#include <array>
#include <exception>
#include <filesystem>
#include <string>
#include <system_error>

namespace bltzr_module {
static bool hasRequiredExports(const ExportsV1& exports)
{
    return exports.apiVersion == kApiVersionV1 && exports.create != nullptr &&
           exports.destroy != nullptr && exports.start != nullptr && exports.stop != nullptr &&
           exports.handleCommand != nullptr;
}

static void clearFailedLoad(std::optional<ExportsV1>& exports,
                            bltzr_platform::DynamicLibrary& library) noexcept
{
    exports.reset();
    library.close();
}

static bool resolveExports(std::uintptr_t entryPointAddress,
                           std::optional<ExportsV1>& exports,
                           std::string& outError)
{
    auto entryPoint = reinterpret_cast<EntryPointFn>(entryPointAddress);
    if (entryPoint == nullptr) {
        outError = "module entry point resolved to null";
        return false;
    }
    try {
        // The ABI returns a module-static table. Borrow it only long enough to copy it.
        const auto exportedTable = entryPoint();
        if (exportedTable == nullptr) {
            outError = "entry point returned null exports";
            return false;
        }
        exports = *exportedTable;
        return true;
    }
    catch (const std::exception& ex) {
        outError = std::string("module entry point threw: ") + ex.what();
    }
    catch (...) {
        outError = "module entry point threw unknown exception";
    }
    return false;
}

static bool createModuleState(const ExportsV1& exports,
                              const HostContextV1& context,
                              CreateResult& result,
                              std::array<char, kErrorBufferSize>& errorBuffer,
                              std::string& outError)
{
    try {
        if (!exports.create(&context, result.rawSlot(), errorBuffer.data(), errorBuffer.size())) {
            outError = errorFromBuffer(errorBuffer, "module create failed");
            return false;
        }
    }
    catch (const std::exception& ex) {
        outError = std::string("module create threw: ") + ex.what();
        return false;
    }
    catch (...) {
        outError = "module create threw unknown exception";
        return false;
    }
    if (!result.hasValue()) {
        outError = "module create returned null state";
        return false;
    }
    return true;
}

static bool startModule(const ExportsV1& exports,
                        const OpaqueState& state,
                        std::array<char, kErrorBufferSize>& errorBuffer,
                        std::string& outError)
{
    try {
        if (!exports.start(state.rawPointer(), errorBuffer.data(), errorBuffer.size())) {
            outError = errorFromBuffer(errorBuffer, "module start failed");
            return false;
        }
    }
    catch (const std::exception& ex) {
        outError = std::string("module start threw: ") + ex.what();
        return false;
    }
    catch (...) {
        outError = "module start threw unknown exception";
        return false;
    }
    return true;
}

bool Handle::load(const std::string& modulePath, const std::string& configPath,
                              std::string_view expectedModuleId, std::string& outError)
{
    if (!m_impl)
        m_impl = std::make_unique<Impl>();
    unload();
    const auto destroyStateNoexcept = [this]() -> bool {
        if (!m_impl->exports.has_value() || !m_impl->state.hasValue()) {
            return true;
        }
        try {
            m_impl->exports->destroy(m_impl->state.rawPointer());
        }
        catch (...) {
            return false;
        }
        m_impl->state.clear();
        return true;
    };
    std::error_code ec;
    const std::filesystem::path requested(modulePath);
    const std::filesystem::path normalized =
        requested.is_absolute() ? requested : std::filesystem::absolute(requested, ec);
    const std::string effectivePath = (!ec ? normalized.string() : modulePath);
    Manifest manifest{};
    if (!Manifest::load(effectivePath, manifest, outError)) {
        return false;
    }
    if (!manifest.validateForLoad(effectivePath, expectedModuleId, outError)) {
        return false;
    }
    std::string moduleDigest;
    if (!computeFileSha256Hex(effectivePath, moduleDigest, outError)) {
        return false;
    }
    if (moduleDigest != manifest.sha256()) {
        outError = "module sha256 mismatch";
        return false;
    }
    if (!m_impl->library.open(effectivePath, outError)) {
        return false;
    }
    std::uintptr_t entryPointAddress = 0u;
    if (!m_impl->library.loadSymbolAddress(kEntryPoint, entryPointAddress, outError)) {
        m_impl->library.close();
        return false;
    }
    if (!resolveExports(entryPointAddress, m_impl->exports, outError)) {
        m_impl->library.close();
        return false;
    }
    if (!hasRequiredExports(*m_impl->exports)) {
        outError = "module exports are incomplete";
        if (m_impl->exports->apiVersion != kApiVersionV1)
            outError = "unsupported module api version";
        clearFailedLoad(m_impl->exports, m_impl->library);
        return false;
    }
    if (m_impl->exports->moduleName == nullptr ||
        manifest.moduleId() != m_impl->exports->moduleName) {
        outError = "module exports do not match manifest";
        clearFailedLoad(m_impl->exports, m_impl->library);
        return false;
    }
    std::array<char, kErrorBufferSize> errorBuffer{};
    const HostContextV1 context{configPath.c_str(), effectivePath.c_str()};
    CreateResult createResult{};
    if (!createModuleState(*m_impl->exports, context, createResult, errorBuffer, outError)) {
        clearFailedLoad(m_impl->exports, m_impl->library);
        return false;
    }
    m_impl->state = createResult.state();
    if (!startModule(*m_impl->exports, m_impl->state, errorBuffer, outError)) {
        (void)destroyStateNoexcept();
        clearFailedLoad(m_impl->exports, m_impl->library);
        return false;
    }
    outError.clear();
    m_impl->path = effectivePath;
    return true;
}
} // namespace bltzr_module
