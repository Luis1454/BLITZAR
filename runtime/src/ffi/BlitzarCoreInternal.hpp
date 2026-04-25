#ifndef GRAVITY_RUNTIME_SRC_FFI_BLITZARCOREINTERNAL_HPP_
#define GRAVITY_RUNTIME_SRC_FFI_BLITZARCOREINTERNAL_HPP_
#include "ffi/BlitzarCoreApi.hpp"
#include "server/SimulationServer.hpp"
#include <cstdint>
#include <mutex>
#include <string>
namespace grav_ffi {
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
} // namespace grav_ffi
struct blitzar_core {
    explicit blitzar_core(const blitzar_core_config_t& config);
    grav_ffi::BlitzarCore impl;
};
#endif // GRAVITY_RUNTIME_SRC_FFI_BLITZARCOREINTERNAL_HPP_
