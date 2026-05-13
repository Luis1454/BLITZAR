/*
 * @file runtime/include/client/module/Boundary.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Runtime public interfaces for protocol, command, client, and FFI boundaries.
 */

#ifndef BLITZAR_RUNTIME_INCLUDE_CLIENT_CLIENTMODULEBOUNDARY_HPP_
#define BLITZAR_RUNTIME_INCLUDE_CLIENT_CLIENTMODULEBOUNDARY_HPP_
#include "client/diagnostics/ErrorBuffer.hpp"
#include <cstddef>
#include <cstdint>

namespace bltzr_module {
class OpaqueState final {
public:
    OpaqueState() noexcept;
    explicit OpaqueState(std::uintptr_t opaqueValue) noexcept;
    [[nodiscard]] static OpaqueState fromRawPointer(void* rawPointer) noexcept;
    [[nodiscard]] bool hasValue() const noexcept;
    [[nodiscard]] std::uintptr_t opaqueValue() const noexcept;
    [[nodiscard]] void* rawPointer() const noexcept;
    void clear() noexcept;

private:
    std::uintptr_t m_opaqueValue;
};

class CreateResult final {
public:
    CreateResult() noexcept;
    [[nodiscard]] bool hasValue() const noexcept;
    [[nodiscard]] void** rawSlot() noexcept;
    [[nodiscard]] OpaqueState state() const noexcept;

private:
    void* m_rawState;
};

class StateSlot final {
public:
    explicit StateSlot(void** slot) noexcept;
    [[nodiscard]] bool isAvailable() const noexcept;
    bool assign(OpaqueState state) const noexcept;

private:
    void** m_slot;
};

class CommandControl final {
public:
    explicit CommandControl(bool* keepRunningFlag) noexcept;
    void requestStop() const noexcept;
    void setContinue() const noexcept;

private:
    bool* m_keepRunningFlag;
};

class Result final {
public:
    Result() noexcept;
    [[nodiscard]] bool keepRunning() const noexcept;
    [[nodiscard]] bool* rawKeepRunningFlag() noexcept;

private:
    bool m_keepRunning;
};
} // namespace bltzr_module
#endif // BLITZAR_RUNTIME_INCLUDE_CLIENT_CLIENTMODULEBOUNDARY_HPP_
