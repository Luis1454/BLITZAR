/*
 * @file runtime/src/client/module/Handle.cpp
 * @author BLITZAR Contributors
 * @project BLITZAR
 * @brief Runtime implementation for protocol, command, client, and FFI boundaries.
 */

#include "client/module/Handle.hpp"
#include "Internal.hpp"
#include <array>
#include <exception>
#include <iostream>
#include <string>
#include <string_view>

namespace bltzr_module {
std::string errorFromBuffer(const std::array<char, kErrorBufferSize>& buffer,
                            std::string_view fallback)
{
    std::string error = buffer.data();
    if (error.empty()) {
        error.assign(fallback.begin(), fallback.end());
    }
    return error;
}

Handle::Handle() : m_impl(std::make_unique<Impl>())
{
}

Handle::~Handle()
{
    unload();
}

Handle::Handle(Handle&& other) noexcept = default;
Handle& Handle::operator=(Handle&& other) noexcept = default;

void Handle::unload() noexcept
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

bool Handle::isLoaded() const noexcept
{
    return m_impl && m_impl->exports != nullptr && m_impl->state.hasValue();
}

std::string_view Handle::moduleName() const noexcept
{
    if (!m_impl || m_impl->exports == nullptr || m_impl->exports->moduleName == nullptr)
        return {};
    return m_impl->exports->moduleName;
}

std::string_view Handle::loadedPath() const noexcept
{
    if (!m_impl)
        return {};
    return m_impl->path;
}

bool Handle::handleCommand(std::string_view commandLine, bool& outKeepRunning,
                                       std::string& outError)
{
    if (!isLoaded()) {
        outError = "module not loaded";
        return false;
    }
    std::array<char, kErrorBufferSize> errorBuffer{};
    std::string command(commandLine);
    Result commandResult{};
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
} // namespace bltzr_module
