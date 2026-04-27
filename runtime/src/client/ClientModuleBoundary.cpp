// File: runtime/src/client/ClientModuleBoundary.cpp
// Purpose: Runtime integration surface for BLITZAR clients and protocols.

#include "client/ClientModuleBoundary.hpp"

namespace grav_module {
/// Description: Executes the ClientModuleOpaqueState operation.
ClientModuleOpaqueState::ClientModuleOpaqueState() noexcept : m_opaqueValue(0u)
{
}

/// Description: Describes the client module opaque state operation contract.
ClientModuleOpaqueState::ClientModuleOpaqueState(std::uintptr_t opaqueValue) noexcept
    : m_opaqueValue(opaqueValue)
{
}

/// Description: Describes the from raw pointer operation contract.
ClientModuleOpaqueState ClientModuleOpaqueState::fromRawPointer(void* rawPointer) noexcept
{
    return ClientModuleOpaqueState(reinterpret_cast<std::uintptr_t>(rawPointer));
}

/// Description: Describes the has value operation contract.
bool ClientModuleOpaqueState::hasValue() const noexcept
{
    return m_opaqueValue != 0u;
}

/// Description: Describes the opaque value operation contract.
std::uintptr_t ClientModuleOpaqueState::opaqueValue() const noexcept
{
    return m_opaqueValue;
}

/// Description: Describes the raw pointer operation contract.
void* ClientModuleOpaqueState::rawPointer() const noexcept
{
    return reinterpret_cast<void*>(m_opaqueValue);
}

/// Description: Describes the clear operation contract.
void ClientModuleOpaqueState::clear() noexcept
{
    m_opaqueValue = 0u;
}

/// Description: Executes the ClientModuleCreateResult operation.
ClientModuleCreateResult::ClientModuleCreateResult() noexcept : m_rawState(nullptr)
{
}

/// Description: Describes the has value operation contract.
bool ClientModuleCreateResult::hasValue() const noexcept
{
    return m_rawState != nullptr;
}

/// Description: Describes the raw slot operation contract.
void** ClientModuleCreateResult::rawSlot() noexcept
{
    return &m_rawState;
}

/// Description: Describes the state operation contract.
ClientModuleOpaqueState ClientModuleCreateResult::state() const noexcept
{
    return ClientModuleOpaqueState::fromRawPointer(m_rawState);
}

/// Description: Executes the ClientModuleStateSlot operation.
ClientModuleStateSlot::ClientModuleStateSlot(void** slot) noexcept : m_slot(slot)
{
}

/// Description: Describes the is available operation contract.
bool ClientModuleStateSlot::isAvailable() const noexcept
{
    return m_slot != nullptr;
}

/// Description: Describes the assign operation contract.
bool ClientModuleStateSlot::assign(ClientModuleOpaqueState state) const noexcept
{
    if (m_slot == nullptr)
        return false;
    *m_slot = state.rawPointer();
    return true;
}

/// Description: Describes the client module command control operation contract.
ClientModuleCommandControl::ClientModuleCommandControl(bool* keepRunningFlag) noexcept
    : m_keepRunningFlag(keepRunningFlag)
{
}

/// Description: Describes the request stop operation contract.
void ClientModuleCommandControl::requestStop() const noexcept
{
    if (m_keepRunningFlag != nullptr) {
        *m_keepRunningFlag = false;
    }
}

/// Description: Describes the set continue operation contract.
void ClientModuleCommandControl::setContinue() const noexcept
{
    if (m_keepRunningFlag != nullptr) {
        *m_keepRunningFlag = true;
    }
}

/// Description: Executes the ClientModuleCommandResult operation.
ClientModuleCommandResult::ClientModuleCommandResult() noexcept : m_keepRunning(true)
{
}

/// Description: Describes the keep running operation contract.
bool ClientModuleCommandResult::keepRunning() const noexcept
{
    return m_keepRunning;
}

/// Description: Describes the raw keep running flag operation contract.
bool* ClientModuleCommandResult::rawKeepRunningFlag() noexcept
{
    return &m_keepRunning;
}
} // namespace grav_module
