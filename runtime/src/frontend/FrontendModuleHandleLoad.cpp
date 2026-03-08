#include "frontend/FrontendModuleHandle.hpp"
#include "frontend/FrontendModuleHandleLoad.hpp"

#include "runtime/src/frontend/FrontendModuleHandleInternal.hpp"

#include <array>
#include <exception>
#include <filesystem>
#include <string>
#include <system_error>

namespace grav_module {

static bool hasRequiredExports(const FrontendModuleExportsV1 *exports)
{
    return exports != nullptr
        && exports->apiVersion == kFrontendModuleApiVersionV1
        && exports->create != nullptr
        && exports->destroy != nullptr
        && exports->start != nullptr
        && exports->stop != nullptr
        && exports->handleCommand != nullptr;
}

bool FrontendModuleHandle::load(const std::string &modulePath, const std::string &configPath, std::string &outError)
{
    if (!m_impl) {
        m_impl = std::make_unique<Impl>();
    }
    unload();

    const auto destroyStateNoexcept = [this]() -> bool {
        if (m_impl->exports == nullptr || !m_impl->state.hasValue()) {
            return true;
        }
        try {
            m_impl->exports->destroy(m_impl->state.rawPointer());
        } catch (...) {
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

    if (!m_impl->library.open(effectivePath, outError)) {
        return false;
    }

    std::uintptr_t entryPointAddress = 0u;
    if (!m_impl->library.loadSymbolAddress(kFrontendModuleEntryPoint, entryPointAddress, outError)) {
        m_impl->library.close();
        return false;
    }

    auto entryPoint = reinterpret_cast<FrontendModuleEntryPointFn>(entryPointAddress);
    if (entryPoint == nullptr) {
        outError = "module entry point resolved to null";
        m_impl->library.close();
        return false;
    }

    try {
        m_impl->exports = entryPoint();
    } catch (const std::exception &ex) {
        outError = std::string("module entry point threw: ") + ex.what();
        m_impl->library.close();
        return false;
    } catch (...) {
        outError = "module entry point threw unknown exception";
        m_impl->library.close();
        return false;
    }

    if (!hasRequiredExports(m_impl->exports)) {
        outError = m_impl->exports == nullptr ? "entry point returned null exports" : "module exports are incomplete";
        if (m_impl->exports != nullptr && m_impl->exports->apiVersion != kFrontendModuleApiVersionV1) {
            outError = "unsupported module api version";
        }
        m_impl->exports = nullptr;
        m_impl->library.close();
        return false;
    }

    std::array<char, kErrorBufferSize> errorBuffer{};
    const FrontendModuleHostContextV1 context{configPath.c_str()};
    FrontendModuleCreateResult createResult{};
    try {
        if (!m_impl->exports->create(&context, createResult.rawSlot(), errorBuffer.data(), errorBuffer.size())) {
            outError = errorFromBuffer(errorBuffer, "module create failed");
            m_impl->exports = nullptr;
            m_impl->library.close();
            return false;
        }
    } catch (const std::exception &ex) {
        outError = std::string("module create threw: ") + ex.what();
        m_impl->exports = nullptr;
        m_impl->library.close();
        return false;
    } catch (...) {
        outError = "module create threw unknown exception";
        m_impl->exports = nullptr;
        m_impl->library.close();
        return false;
    }
    if (!createResult.hasValue()) {
        outError = "module create returned null state";
        m_impl->exports = nullptr;
        m_impl->library.close();
        return false;
    }
    m_impl->state = createResult.state();

    try {
        if (!m_impl->exports->start(m_impl->state.rawPointer(), errorBuffer.data(), errorBuffer.size())) {
            outError = errorFromBuffer(errorBuffer, "module start failed");
            (void)destroyStateNoexcept();
            m_impl->exports = nullptr;
            m_impl->library.close();
            return false;
        }
    } catch (const std::exception &ex) {
        outError = std::string("module start threw: ") + ex.what();
        (void)destroyStateNoexcept();
        m_impl->exports = nullptr;
        m_impl->library.close();
        return false;
    } catch (...) {
        outError = "module start threw unknown exception";
        (void)destroyStateNoexcept();
        m_impl->exports = nullptr;
        m_impl->library.close();
        return false;
    }

    outError.clear();
    m_impl->path = effectivePath;
    return true;
}

} // namespace grav_module
