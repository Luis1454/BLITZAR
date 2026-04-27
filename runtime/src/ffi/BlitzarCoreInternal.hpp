// File: runtime/src/ffi/BlitzarCoreInternal.hpp
// Purpose: Runtime integration surface for BLITZAR clients and protocols.

#ifndef GRAVITY_RUNTIME_SRC_FFI_BLITZARCOREINTERNAL_HPP_
#define GRAVITY_RUNTIME_SRC_FFI_BLITZARCOREINTERNAL_HPP_
#include "ffi/BlitzarCoreApi.hpp"
#include "server/SimulationServer.hpp"
#include <cstdint>
#include <mutex>
#include <string>
namespace grav_ffi {
/// Description: Defines the BlitzarCore data or behavior contract.
class BlitzarCore final {
public:
    /// Description: Executes the BlitzarCore operation.
    explicit BlitzarCore(const blitzar_core_config_t& config);
    /// Description: Releases resources owned by BlitzarCore.
    ~BlitzarCore();
    /// Description: Executes the applyConfig operation.
    blitzar_core_result_t applyConfig(const blitzar_core_config_t& config);
    /// Description: Executes the runSteps operation.
    blitzar_core_result_t runSteps(std::uint32_t steps, std::uint32_t timeoutMs);
    /// Description: Executes the getStatus operation.
    blitzar_core_result_t getStatus(blitzar_core_status_t& outStatus) const;
    blitzar_core_result_t getSnapshot(std::size_t maxPoints,
                                      blitzar_core_snapshot_t& outSnapshot) const;
    /// Description: Executes the loadState operation.
    blitzar_core_result_t loadState(const char* path, const char* format, std::uint32_t timeoutMs);
    blitzar_core_result_t exportState(const char* path, const char* format,
                                      std::uint32_t timeoutMs);
    /// Description: Executes the copyLastError operation.
    std::size_t copyLastError(char* buffer, std::size_t capacity) const;

private:
    /// Description: Executes the waitForSnapshot operation.
    blitzar_core_result_t waitForSnapshot(std::uint32_t timeoutMs) const;
    blitzar_core_result_t waitForAppliedConfig(const blitzar_core_config_t& config,
                                               std::uint32_t timeoutMs) const;
    blitzar_core_result_t waitForStepTarget(std::uint64_t expectedSteps,
                                            std::uint32_t timeoutMs) const;
    /// Description: Executes the waitForFile operation.
    blitzar_core_result_t waitForFile(const char* path, std::uint32_t timeoutMs) const;
    /// Description: Executes the setError operation.
    void setError(const std::string& message) const;
    /// Description: Executes the clearError operation.
    void clearError() const;
    SimulationServer _server;
    InitialStateConfig _initialStateConfig;
    mutable std::mutex _errorMutex;
    mutable std::string _lastError;
};
} // namespace grav_ffi
/// Description: Defines the blitzar_core data or behavior contract.
struct blitzar_core {
    /// Description: Executes the blitzar_core operation.
    explicit blitzar_core(const blitzar_core_config_t& config);
    grav_ffi::BlitzarCore impl;
};
#endif // GRAVITY_RUNTIME_SRC_FFI_BLITZARCOREINTERNAL_HPP_
