// File: runtime/src/client/ClientModuleHandle.cpp
// Purpose: Runtime integration surface for BLITZAR clients and protocols.

#include "client/ClientModuleHandle.hpp"
#include "runtime/src/client/ClientModuleHandleInternal.hpp"
#include <array>
#include <exception>
#include <iostream>
#include <string>
#include <string_view>
namespace grav_module {
std::string errorFromBuffer(const std::array<char, kErrorBufferSize>& buffer,
                            std::string_view fallback)
{
    std::string error = buffer.data();
    if (error.empty()) {
        error.assign(fallback.begin(), fallback.end());
    }
    return error;
}
/// Description: Executes the ClientModuleHandle operation.
ClientModuleHandle::ClientModuleHandle() : m_impl(std::make_unique<Impl>())
{
}
/// Description: Releases resources owned by ClientModuleHandle.
ClientModuleHandle::~ClientModuleHandle()
{
    /// Description: Executes the unload operation.
    unload();
}
ClientModuleHandle::ClientModuleHandle(ClientModuleHandle&& other) noexcept = default;
ClientModuleHandle& ClientModuleHandle::operator=(ClientModuleHandle&& other) noexcept = default;
void ClientModuleHandle::unload() noexcept
{
    if (!m_impl) {
        return;
    }
    if (m_impl->exports != nullptr && m_impl->state.hasValue()) {
        try {
            m_impl->exports->stop(m_impl->state.rawPointer());
        }
        catch (const std::exception& ex) {
            std::cerr << "[client-host] module stop threw: " << ex.what() << "\n";
        }
        catch (...) {
            std::cerr << "[client-host] module stop threw unknown exception\n";
        }
        try {
            m_impl->exports->destroy(m_impl->state.rawPointer());
        }
        catch (const std::exception& ex) {
            std::cerr << "[client-host] module destroy threw: " << ex.what() << "\n";
        }
        catch (...) {
            std::cerr << "[client-host] module destroy threw unknown exception\n";
        }
    }
    m_impl->exports = nullptr;
    m_impl->state.clear();
    m_impl->path.clear();
    m_impl->library.close();
}
bool ClientModuleHandle::isLoaded() const noexcept
{
    return m_impl && m_impl->exports != nullptr && m_impl->state.hasValue();
}
std::string_view ClientModuleHandle::moduleName() const noexcept
{
    if (!m_impl || m_impl->exports == nullptr || m_impl->exports->moduleName == nullptr)
        return {};
    return m_impl->exports->moduleName;
}
std::string_view ClientModuleHandle::loadedPath() const noexcept
{
    if (!m_impl)
        return {};
    return m_impl->path;
}
bool ClientModuleHandle::handleCommand(std::string_view commandLine, bool& outKeepRunning,
                                       std::string& outError)
{
    if (!isLoaded()) {
        outError = "module not loaded";
        return false;
    }
    std::array<char, kErrorBufferSize> errorBuffer{};
    /// Description: Executes the command operation.
    std::string command(commandLine);
    ClientModuleCommandResult commandResult{};
    outKeepRunning = commandResult.keepRunning();
    bool commandOk = false;
    try {
        commandOk = m_impl->exports->handleCommand(m_impl->state.rawPointer(), command.c_str(),
                                                   commandResult.rawKeepRunningFlag(),
                                                   errorBuffer.data(), errorBuffer.size());
    }
    catch (const std::exception& ex) {
        outError = std::string("module command threw: ") + ex.what();
        return false;
    }
    catch (...) {
        outError = "module command threw unknown exception";
        return false;
    }
    outKeepRunning = commandResult.keepRunning();
    if (!commandOk) {
        outError = errorFromBuffer(errorBuffer, "module command failed");
        return false;
    }
    outError.clear();
    return true;
}
} // namespace grav_module
