/*
 * @file runtime/src/client/module/Boundary.cpp
 * @author BLITZAR Contributors
 * @project BLITZAR
 * @brief Runtime implementation for protocol, command, client, and FFI boundaries.
 */

#include "client/module/Boundary.hpp"

namespace bltzr_module {
OpaqueState::OpaqueState() noexcept : m_opaqueValue(0u)
{
}

OpaqueState::OpaqueState(std::uintptr_t opaqueValue) noexcept
    : m_opaqueValue(opaqueValue)
{
}

OpaqueState OpaqueState::fromRawPointer(void* rawPointer) noexcept
{
    return OpaqueState(reinterpret_cast<std::uintptr_t>(rawPointer));
}

bool OpaqueState::hasValue() const noexcept
{
    return m_opaqueValue != 0u;
}

std::uintptr_t OpaqueState::opaqueValue() const noexcept
{
    return m_opaqueValue;
}

void* OpaqueState::rawPointer() const noexcept
{
    return reinterpret_cast<void*>(m_opaqueValue);
}

void OpaqueState::clear() noexcept
{
    m_opaqueValue = 0u;
}

CreateResult::CreateResult() noexcept : m_rawState(nullptr)
{
}

bool CreateResult::hasValue() const noexcept
{
    return m_rawState != nullptr;
}

void** CreateResult::rawSlot() noexcept
{
    return &m_rawState;
}

OpaqueState CreateResult::state() const noexcept
{
    return OpaqueState::fromRawPointer(m_rawState);
}

StateSlot::StateSlot(void** slot) noexcept : m_slot(slot)
{
}

bool StateSlot::isAvailable() const noexcept
{
    return m_slot != nullptr;
}

bool StateSlot::assign(OpaqueState state) const noexcept
{
    if (m_slot == nullptr)
        return false;
    *m_slot = state.rawPointer();
    return true;
}

CommandControl::CommandControl(bool* keepRunningFlag) noexcept
    : m_keepRunningFlag(keepRunningFlag)
{
}

void CommandControl::requestStop() const noexcept
{
    if (m_keepRunningFlag != nullptr) {
        *m_keepRunningFlag = false;
    }
}

void CommandControl::setContinue() const noexcept
{
    if (m_keepRunningFlag != nullptr) {
        *m_keepRunningFlag = true;
    }
}

Result::Result() noexcept : m_keepRunning(true)
{
}

bool Result::keepRunning() const noexcept
{
    return m_keepRunning;
}

bool* Result::rawKeepRunningFlag() noexcept
{
    return &m_keepRunning;
}
} // namespace bltzr_module
