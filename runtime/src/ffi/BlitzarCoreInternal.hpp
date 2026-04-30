/*
 * @file runtime/src/ffi/BlitzarCoreInternal.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Runtime implementation for protocol, command, client, and FFI boundaries.
 */

#ifndef BLITZAR_RUNTIME_SRC_FFI_BLITZARCOREINTERNAL_HPP_
#define BLITZAR_RUNTIME_SRC_FFI_BLITZARCOREINTERNAL_HPP_
#include "ffi/BlitzarCoreApi.hpp"
#include "server/SimulationServer.hpp"
#include <cstdint>
#include <mutex>
#include <string>

namespace bltzr_ffi {
class BlitzarCore final {
public:
    explicit BlitzarCore(const blitzar_core_config_t& config);
    ~BlitzarCore();
    blitzar_core_result_t applyConfig(const blitzar_core_config_t& config);
    blitzar_core_result_t runSteps(std::uint32_t steps, std::uint32_t timeoutMs);
    blitzar_core_result_t getStatus(blitzar_core_status_t& outStatus) const;
    blitzar_core_result_t getSnapshot(std::size_t maxPoints,
                                      blitzar_core_snapshot_t& outSnapshot) const;
    blitzar_core_result_t loadState(const char* path, const char* format, std::uint32_t timeoutMs);
    blitzar_core_result_t exportState(const char* path, const char* format,
                                      std::uint32_t timeoutMs);
    std::size_t copyLastError(char* buffer, std::size_t capacity) const;

private:
    blitzar_core_result_t waitForSnapshot(std::uint32_t timeoutMs) const;
    blitzar_core_result_t waitForAppliedConfig(const blitzar_core_config_t& config,
                                               std::uint32_t timeoutMs) const;
    blitzar_core_result_t waitForStepTarget(std::uint64_t expectedSteps,
                                            std::uint32_t timeoutMs) const;
    blitzar_core_result_t waitForFile(const char* path, std::uint32_t timeoutMs) const;
    void setError(const std::string& message) const;
    void clearError() const;
    SimulationServer _server;
    InitialStateConfig _initialStateConfig;
    mutable std::mutex _errorMutex;
    mutable std::string _lastError;
};
} // namespace bltzr_ffi

/*
 * @brief Defines the blitzar core type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
struct blitzar_core {
    /*
     * @brief Documents the blitzar core operation contract.
     * @param config Input value used by this contract.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    explicit blitzar_core(const blitzar_core_config_t& config);
    bltzr_ffi::BlitzarCore impl;
};
#endif // BLITZAR_RUNTIME_SRC_FFI_BLITZARCOREINTERNAL_HPP_
