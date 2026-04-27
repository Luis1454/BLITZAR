// File: runtime/src/client/ClientModuleBoundary.cpp
// Purpose: Runtime integration surface for BLITZAR clients and protocols.

#include "client/ClientModuleBoundary.hpp"
namespace grav_module {
/// Description: Executes the ClientModuleOpaqueState operation.
ClientModuleOpaqueState::ClientModuleOpaqueState() noexcept : m_opaqueValue(0u)
{
}
ClientModuleOpaqueState::ClientModuleOpaqueState(std::uintptr_t opaqueValue) noexcept
    : m_opaqueValue(opaqueValue)
{
}
ClientModuleOpaqueState ClientModuleOpaqueState::fromRawPointer(void* rawPointer) noexcept
{
    return ClientModuleOpaqueState(reinterpret_cast<std::uintptr_t>(rawPointer));
}
bool ClientModuleOpaqueState::hasValue() const noexcept
{
    return m_opaqueValue != 0u;
}
std::uintptr_t ClientModuleOpaqueState::opaqueValue() const noexcept
{
    return m_opaqueValue;
}
void* ClientModuleOpaqueState::rawPointer() const noexcept
{
    return reinterpret_cast<void*>(m_opaqueValue);
}
void ClientModuleOpaqueState::clear() noexcept
{
    m_opaqueValue = 0u;
}
/// Description: Executes the ClientModuleCreateResult operation.
ClientModuleCreateResult::ClientModuleCreateResult() noexcept : m_rawState(nullptr)
{
}
bool ClientModuleCreateResult::hasValue() const noexcept
{
    return m_rawState != nullptr;
}
void** ClientModuleCreateResult::rawSlot() noexcept
{
    return &m_rawState;
}
ClientModuleOpaqueState ClientModuleCreateResult::state() const noexcept
{
    return ClientModuleOpaqueState::fromRawPointer(m_rawState);
}
/// Description: Executes the ClientModuleStateSlot operation.
ClientModuleStateSlot::ClientModuleStateSlot(void** slot) noexcept : m_slot(slot)
{
}
bool ClientModuleStateSlot::isAvailable() const noexcept
{
    return m_slot != nullptr;
}
bool ClientModuleStateSlot::assign(ClientModuleOpaqueState state) const noexcept
{
    if (m_slot == nullptr)
        return false;
    *m_slot = state.rawPointer();
    return true;
}
ClientModuleCommandControl::ClientModuleCommandControl(bool* keepRunningFlag) noexcept
    : m_keepRunningFlag(keepRunningFlag)
{
}
void ClientModuleCommandControl::requestStop() const noexcept
{
    if (m_keepRunningFlag != nullptr) {
        *m_keepRunningFlag = false;
    }
}
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
bool ClientModuleCommandResult::keepRunning() const noexcept
{
    return m_keepRunning;
}
bool* ClientModuleCommandResult::rawKeepRunningFlag() noexcept
{
    return &m_keepRunning;
}
} // namespace grav_module
