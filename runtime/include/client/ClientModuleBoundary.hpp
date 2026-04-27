// File: runtime/include/client/ClientModuleBoundary.hpp
// Purpose: Runtime integration surface for BLITZAR clients and protocols.

#ifndef GRAVITY_RUNTIME_INCLUDE_CLIENT_CLIENTMODULEBOUNDARY_HPP_
#define GRAVITY_RUNTIME_INCLUDE_CLIENT_CLIENTMODULEBOUNDARY_HPP_
#include "client/ErrorBuffer.hpp"
#include <cstddef>
#include <cstdint>
namespace grav_module {
class ClientModuleOpaqueState final {
public:
    ClientModuleOpaqueState() noexcept;
    explicit ClientModuleOpaqueState(std::uintptr_t opaqueValue) noexcept;
    [[nodiscard]] static ClientModuleOpaqueState fromRawPointer(void* rawPointer) noexcept;
    [[nodiscard]] bool hasValue() const noexcept;
    [[nodiscard]] std::uintptr_t opaqueValue() const noexcept;
    [[nodiscard]] void* rawPointer() const noexcept;
    void clear() noexcept;

private:
    std::uintptr_t m_opaqueValue;
};
class ClientModuleCreateResult final {
public:
    ClientModuleCreateResult() noexcept;
    [[nodiscard]] bool hasValue() const noexcept;
    [[nodiscard]] void** rawSlot() noexcept;
    [[nodiscard]] ClientModuleOpaqueState state() const noexcept;

private:
    void* m_rawState;
};
class ClientModuleStateSlot final {
public:
    explicit ClientModuleStateSlot(void** slot) noexcept;
    [[nodiscard]] bool isAvailable() const noexcept;
    bool assign(ClientModuleOpaqueState state) const noexcept;

private:
    void** m_slot;
};
class ClientModuleCommandControl final {
public:
    explicit ClientModuleCommandControl(bool* keepRunningFlag) noexcept;
    void requestStop() const noexcept;
    void setContinue() const noexcept;

private:
    bool* m_keepRunningFlag;
};
class ClientModuleCommandResult final {
public:
    ClientModuleCommandResult() noexcept;
    [[nodiscard]] bool keepRunning() const noexcept;
    [[nodiscard]] bool* rawKeepRunningFlag() noexcept;

private:
    bool m_keepRunning;
};
} // namespace grav_module
#endif // GRAVITY_RUNTIME_INCLUDE_CLIENT_CLIENTMODULEBOUNDARY_HPP_
