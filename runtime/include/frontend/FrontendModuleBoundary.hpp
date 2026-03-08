#ifndef GRAVITY_SIM_FRONTENDMODULEBOUNDARY_HPP
#define GRAVITY_SIM_FRONTENDMODULEBOUNDARY_HPP

#include "frontend/ErrorBuffer.hpp"

#include <cstddef>
#include <cstdint>

namespace grav_module {

class FrontendModuleOpaqueState final {
public:
    FrontendModuleOpaqueState() noexcept;
    explicit FrontendModuleOpaqueState(std::uintptr_t opaqueValue) noexcept;

    [[nodiscard]] static FrontendModuleOpaqueState fromRawPointer(void *rawPointer) noexcept;
    [[nodiscard]] bool hasValue() const noexcept;
    [[nodiscard]] std::uintptr_t opaqueValue() const noexcept;
    [[nodiscard]] void *rawPointer() const noexcept;
    void clear() noexcept;

private:
    std::uintptr_t m_opaqueValue;
};

class FrontendModuleCreateResult final {
public:
    FrontendModuleCreateResult() noexcept;

    [[nodiscard]] bool hasValue() const noexcept;
    [[nodiscard]] void **rawSlot() noexcept;
    [[nodiscard]] FrontendModuleOpaqueState state() const noexcept;

private:
    void *m_rawState;
};

class FrontendModuleStateSlot final {
public:
    explicit FrontendModuleStateSlot(void **slot) noexcept;

    [[nodiscard]] bool isAvailable() const noexcept;
    bool assign(FrontendModuleOpaqueState state) const noexcept;

private:
    void **m_slot;
};

class FrontendModuleCommandControl final {
public:
    explicit FrontendModuleCommandControl(bool *keepRunningFlag) noexcept;

    void requestStop() const noexcept;
    void setContinue() const noexcept;

private:
    bool *m_keepRunningFlag;
};

class FrontendModuleCommandResult final {
public:
    FrontendModuleCommandResult() noexcept;

    [[nodiscard]] bool keepRunning() const noexcept;
    [[nodiscard]] bool *rawKeepRunningFlag() noexcept;

private:
    bool m_keepRunning;
};

} // namespace grav_module

#endif // GRAVITY_SIM_FRONTENDMODULEBOUNDARY_HPP
