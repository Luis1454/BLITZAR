#include "frontend/FrontendModuleBoundary.hpp"

namespace grav_module {

FrontendModuleOpaqueState::FrontendModuleOpaqueState() noexcept : m_opaqueValue(0u)
{
}

FrontendModuleOpaqueState::FrontendModuleOpaqueState(std::uintptr_t opaqueValue) noexcept : m_opaqueValue(opaqueValue)
{
}

FrontendModuleOpaqueState FrontendModuleOpaqueState::fromRawPointer(void *rawPointer) noexcept
{
    return FrontendModuleOpaqueState(reinterpret_cast<std::uintptr_t>(rawPointer));
}

bool FrontendModuleOpaqueState::hasValue() const noexcept
{
    return m_opaqueValue != 0u;
}

std::uintptr_t FrontendModuleOpaqueState::opaqueValue() const noexcept
{
    return m_opaqueValue;
}

void *FrontendModuleOpaqueState::rawPointer() const noexcept
{
    return reinterpret_cast<void *>(m_opaqueValue);
}

void FrontendModuleOpaqueState::clear() noexcept
{
    m_opaqueValue = 0u;
}

FrontendModuleCreateResult::FrontendModuleCreateResult() noexcept : m_rawState(nullptr)
{
}

bool FrontendModuleCreateResult::hasValue() const noexcept
{
    return m_rawState != nullptr;
}

void **FrontendModuleCreateResult::rawSlot() noexcept
{
    return &m_rawState;
}

FrontendModuleOpaqueState FrontendModuleCreateResult::state() const noexcept
{
    return FrontendModuleOpaqueState::fromRawPointer(m_rawState);
}

FrontendModuleStateSlot::FrontendModuleStateSlot(void **slot) noexcept : m_slot(slot)
{
}

bool FrontendModuleStateSlot::isAvailable() const noexcept
{
    return m_slot != nullptr;
}

bool FrontendModuleStateSlot::assign(FrontendModuleOpaqueState state) const noexcept
{
    if (m_slot == nullptr) {
        return false;
    }
    *m_slot = state.rawPointer();
    return true;
}

FrontendModuleCommandControl::FrontendModuleCommandControl(bool *keepRunningFlag) noexcept
    : m_keepRunningFlag(keepRunningFlag)
{
}

void FrontendModuleCommandControl::requestStop() const noexcept
{
    if (m_keepRunningFlag != nullptr) {
        *m_keepRunningFlag = false;
    }
}

void FrontendModuleCommandControl::setContinue() const noexcept
{
    if (m_keepRunningFlag != nullptr) {
        *m_keepRunningFlag = true;
    }
}

FrontendModuleCommandResult::FrontendModuleCommandResult() noexcept : m_keepRunning(true)
{
}

bool FrontendModuleCommandResult::keepRunning() const noexcept
{
    return m_keepRunning;
}

bool *FrontendModuleCommandResult::rawKeepRunningFlag() noexcept
{
    return &m_keepRunning;
}

} // namespace grav_module
