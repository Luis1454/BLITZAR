#include "sim/FrontendModuleHandle.hpp"

#include "sim/DynamicLibrary.hpp"
#include "sim/FrontendModuleApi.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <system_error>

namespace sim::module {
namespace {

constexpr std::size_t kErrorBufferSize = 1024u;

std::string errorFromBuffer(const std::array<char, kErrorBufferSize> &buffer, std::string_view fallback)
{
    std::string error = buffer.data();
    if (error.empty()) {
        error.assign(fallback.begin(), fallback.end());
    }
    return error;
}

void *toRawState(std::uintptr_t opaque)
{
    return reinterpret_cast<void *>(opaque);
}

} // namespace

struct FrontendModuleHandle::Impl {
    sim::platform::DynamicLibrary library{};
    const FrontendModuleExportsV1 *exports = nullptr;
    std::uintptr_t stateOpaque = 0u;
    std::string path;
};

FrontendModuleHandle::FrontendModuleHandle() : m_impl(std::make_unique<Impl>())
{
}

FrontendModuleHandle::~FrontendModuleHandle()
{
    unload();
}

FrontendModuleHandle::FrontendModuleHandle(FrontendModuleHandle &&other) noexcept = default;
FrontendModuleHandle &FrontendModuleHandle::operator=(FrontendModuleHandle &&other) noexcept = default;

bool FrontendModuleHandle::load(const std::string &modulePath, const std::string &configPath, std::string &outError)
{
    if (!m_impl) {
        m_impl = std::make_unique<Impl>();
    }
    unload();

    std::error_code ec;
    const std::filesystem::path requested(modulePath);
    const std::filesystem::path normalized =
        requested.is_absolute() ? requested : std::filesystem::absolute(requested, ec);
    const std::string effectivePath = (!ec ? normalized.string() : modulePath);

    if (!m_impl->library.open(effectivePath, outError)) {
        return false;
    }

    FrontendModuleEntryPointFn entryPoint =
        m_impl->library.loadSymbol<FrontendModuleEntryPointFn>(kFrontendModuleEntryPoint, outError);
    if (entryPoint == nullptr) {
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

    if (m_impl->exports == nullptr) {
        outError = "entry point returned null exports";
        m_impl->library.close();
        return false;
    }
    if (m_impl->exports->apiVersion != kFrontendModuleApiVersionV1) {
        outError = "unsupported module api version";
        m_impl->library.close();
        return false;
    }
    if (m_impl->exports->create == nullptr || m_impl->exports->destroy == nullptr
        || m_impl->exports->start == nullptr || m_impl->exports->stop == nullptr
        || m_impl->exports->handleCommand == nullptr) {
        outError = "module exports are incomplete";
        m_impl->library.close();
        return false;
    }

    std::array<char, kErrorBufferSize> errorBuffer{};
    const FrontendModuleHostContextV1 context{configPath.c_str()};
    void *rawState = nullptr;

    try {
        if (!m_impl->exports->create(&context, &rawState, errorBuffer.data(), errorBuffer.size())) {
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

    m_impl->stateOpaque = reinterpret_cast<std::uintptr_t>(rawState);

    try {
        if (!m_impl->exports->start(toRawState(m_impl->stateOpaque), errorBuffer.data(), errorBuffer.size())) {
            outError = errorFromBuffer(errorBuffer, "module start failed");
            m_impl->exports->destroy(toRawState(m_impl->stateOpaque));
            m_impl->stateOpaque = 0u;
            m_impl->exports = nullptr;
            m_impl->library.close();
            return false;
        }
    } catch (const std::exception &ex) {
        outError = std::string("module start threw: ") + ex.what();
        try {
            m_impl->exports->destroy(toRawState(m_impl->stateOpaque));
        } catch (...) {
        }
        m_impl->stateOpaque = 0u;
        m_impl->exports = nullptr;
        m_impl->library.close();
        return false;
    } catch (...) {
        outError = "module start threw unknown exception";
        try {
            m_impl->exports->destroy(toRawState(m_impl->stateOpaque));
        } catch (...) {
        }
        m_impl->stateOpaque = 0u;
        m_impl->exports = nullptr;
        m_impl->library.close();
        return false;
    }

    outError.clear();
    m_impl->path = effectivePath;
    return true;
}

void FrontendModuleHandle::unload() noexcept
{
    if (!m_impl) {
        return;
    }

    if (m_impl->exports != nullptr && m_impl->stateOpaque != 0u) {
        try {
            m_impl->exports->stop(toRawState(m_impl->stateOpaque));
        } catch (const std::exception &ex) {
            std::cerr << "[module-host] module stop threw: " << ex.what() << "\n";
        } catch (...) {
            std::cerr << "[module-host] module stop threw unknown exception\n";
        }
        try {
            m_impl->exports->destroy(toRawState(m_impl->stateOpaque));
        } catch (const std::exception &ex) {
            std::cerr << "[module-host] module destroy threw: " << ex.what() << "\n";
        } catch (...) {
            std::cerr << "[module-host] module destroy threw unknown exception\n";
        }
    }

    m_impl->exports = nullptr;
    m_impl->stateOpaque = 0u;
    m_impl->path.clear();
    m_impl->library.close();
}

bool FrontendModuleHandle::isLoaded() const noexcept
{
    return m_impl && m_impl->exports != nullptr && m_impl->stateOpaque != 0u;
}

std::string_view FrontendModuleHandle::moduleName() const noexcept
{
    if (!m_impl || m_impl->exports == nullptr || m_impl->exports->moduleName == nullptr) {
        return {};
    }
    return m_impl->exports->moduleName;
}

std::string_view FrontendModuleHandle::loadedPath() const noexcept
{
    if (!m_impl) {
        return {};
    }
    return m_impl->path;
}

bool FrontendModuleHandle::handleCommand(std::string_view commandLine, bool &outKeepRunning, std::string &outError)
{
    if (!isLoaded()) {
        outError = "module not loaded";
        return false;
    }

    std::array<char, kErrorBufferSize> errorBuffer{};
    std::string command(commandLine);
    outKeepRunning = true;

    bool commandOk = false;
    try {
        commandOk = m_impl->exports->handleCommand(
            toRawState(m_impl->stateOpaque),
            command.c_str(),
            &outKeepRunning,
            errorBuffer.data(),
            errorBuffer.size());
    } catch (const std::exception &ex) {
        outError = std::string("module command threw: ") + ex.what();
        return false;
    } catch (...) {
        outError = "module command threw unknown exception";
        return false;
    }

    if (!commandOk) {
        outError = errorFromBuffer(errorBuffer, "module command failed");
        return false;
    }

    outError.clear();
    return true;
}

} // namespace sim::module
