// File: runtime/include/client/ClientModuleBoundary.hpp
// Purpose: Runtime integration surface for BLITZAR clients and protocols.

#ifndef GRAVITY_RUNTIME_INCLUDE_CLIENT_CLIENTMODULEBOUNDARY_HPP_
#define GRAVITY_RUNTIME_INCLUDE_CLIENT_CLIENTMODULEBOUNDARY_HPP_
#include "client/ErrorBuffer.hpp"
#include <cstddef>
#include <cstdint>

namespace grav_module {
/// Description: Defines the ClientModuleOpaqueState data or behavior contract.
class ClientModuleOpaqueState final {
public:
    /// Description: Describes the client module opaque state operation contract.
    ClientModuleOpaqueState() noexcept;
    /// Description: Describes the client module opaque state operation contract.
    explicit ClientModuleOpaqueState(std::uintptr_t opaqueValue) noexcept;
    /// Description: Describes the from raw pointer operation contract.
    [[nodiscard]] static ClientModuleOpaqueState fromRawPointer(void* rawPointer) noexcept;
    /// Description: Describes the has value operation contract.
    [[nodiscard]] bool hasValue() const noexcept;
    /// Description: Describes the opaque value operation contract.
    [[nodiscard]] std::uintptr_t opaqueValue() const noexcept;
    /// Description: Describes the raw pointer operation contract.
    [[nodiscard]] void* rawPointer() const noexcept;
    /// Description: Describes the clear operation contract.
    void clear() noexcept;

private:
    std::uintptr_t m_opaqueValue;
};

/// Description: Defines the ClientModuleCreateResult data or behavior contract.
class ClientModuleCreateResult final {
public:
    /// Description: Describes the client module create result operation contract.
    ClientModuleCreateResult() noexcept;
    /// Description: Describes the has value operation contract.
    [[nodiscard]] bool hasValue() const noexcept;
    /// Description: Describes the raw slot operation contract.
    [[nodiscard]] void** rawSlot() noexcept;
    /// Description: Describes the state operation contract.
    [[nodiscard]] ClientModuleOpaqueState state() const noexcept;

private:
    void* m_rawState;
};

/// Description: Defines the ClientModuleStateSlot data or behavior contract.
class ClientModuleStateSlot final {
public:
    /// Description: Describes the client module state slot operation contract.
    explicit ClientModuleStateSlot(void** slot) noexcept;
    /// Description: Describes the is available operation contract.
    [[nodiscard]] bool isAvailable() const noexcept;
    /// Description: Describes the assign operation contract.
    bool assign(ClientModuleOpaqueState state) const noexcept;

private:
    void** m_slot;
};

/// Description: Defines the ClientModuleCommandControl data or behavior contract.
class ClientModuleCommandControl final {
public:
    /// Description: Describes the client module command control operation contract.
    explicit ClientModuleCommandControl(bool* keepRunningFlag) noexcept;
    /// Description: Describes the request stop operation contract.
    void requestStop() const noexcept;
    /// Description: Describes the set continue operation contract.
    void setContinue() const noexcept;

private:
    bool* m_keepRunningFlag;
};

/// Description: Defines the ClientModuleCommandResult data or behavior contract.
class ClientModuleCommandResult final {
public:
    /// Description: Describes the client module command result operation contract.
    ClientModuleCommandResult() noexcept;
    /// Description: Describes the keep running operation contract.
    [[nodiscard]] bool keepRunning() const noexcept;
    /// Description: Describes the raw keep running flag operation contract.
    [[nodiscard]] bool* rawKeepRunningFlag() noexcept;

private:
    bool m_keepRunning;
};
} // namespace grav_module
#endif // GRAVITY_RUNTIME_INCLUDE_CLIENT_CLIENTMODULEBOUNDARY_HPP_
