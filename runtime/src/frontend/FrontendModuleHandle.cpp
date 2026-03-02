#include "frontend/FrontendModuleHandle.hpp"

#include "runtime/src/frontend/FrontendModuleHandleInternal.hpp"

#include <array>
#include <exception>
#include <iostream>
#include <string>
#include <string_view>

namespace grav_module {

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

FrontendModuleHandle::FrontendModuleHandle() : m_impl(std::make_unique<Impl>())
{
}

FrontendModuleHandle::~FrontendModuleHandle()
{
    unload();
}

FrontendModuleHandle::FrontendModuleHandle(FrontendModuleHandle &&other) noexcept = default;
FrontendModuleHandle &FrontendModuleHandle::operator=(FrontendModuleHandle &&other) noexcept = default;

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

} // namespace grav_module
