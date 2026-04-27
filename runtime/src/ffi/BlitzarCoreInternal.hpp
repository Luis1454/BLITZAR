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
    /// Description: Describes the blitzar core operation contract.
    explicit BlitzarCore(const blitzar_core_config_t& config);
    /// Description: Releases resources owned by BlitzarCore.
    ~BlitzarCore();
    /// Description: Describes the apply config operation contract.
    blitzar_core_result_t applyConfig(const blitzar_core_config_t& config);
    /// Description: Describes the run steps operation contract.
    blitzar_core_result_t runSteps(std::uint32_t steps, std::uint32_t timeoutMs);
    /// Description: Describes the get status operation contract.
    blitzar_core_result_t getStatus(blitzar_core_status_t& outStatus) const;
    /// Description: Describes the get snapshot operation contract.
    blitzar_core_result_t getSnapshot(std::size_t maxPoints,
                                      blitzar_core_snapshot_t& outSnapshot) const;
    /// Description: Describes the load state operation contract.
    blitzar_core_result_t loadState(const char* path, const char* format, std::uint32_t timeoutMs);
    /// Description: Describes the export state operation contract.
    blitzar_core_result_t exportState(const char* path, const char* format,
                                      std::uint32_t timeoutMs);
    /// Description: Describes the copy last error operation contract.
    std::size_t copyLastError(char* buffer, std::size_t capacity) const;

private:
    /// Description: Describes the wait for snapshot operation contract.
    blitzar_core_result_t waitForSnapshot(std::uint32_t timeoutMs) const;
    /// Description: Describes the wait for applied config operation contract.
    blitzar_core_result_t waitForAppliedConfig(const blitzar_core_config_t& config,
                                               std::uint32_t timeoutMs) const;
    /// Description: Describes the wait for step target operation contract.
    blitzar_core_result_t waitForStepTarget(std::uint64_t expectedSteps,
                                            std::uint32_t timeoutMs) const;
    /// Description: Describes the wait for file operation contract.
    blitzar_core_result_t waitForFile(const char* path, std::uint32_t timeoutMs) const;
    /// Description: Describes the set error operation contract.
    void setError(const std::string& message) const;
    /// Description: Describes the clear error operation contract.
    void clearError() const;
    SimulationServer _server;
    InitialStateConfig _initialStateConfig;
    mutable std::mutex _errorMutex;
    mutable std::string _lastError;
};
} // namespace grav_ffi

/// Description: Defines the blitzar_core data or behavior contract.
struct blitzar_core {
    /// Description: Describes the blitzar core operation contract.
    explicit blitzar_core(const blitzar_core_config_t& config);
    grav_ffi::BlitzarCore impl;
};
#endif // GRAVITY_RUNTIME_SRC_FFI_BLITZARCOREINTERNAL_HPP_
