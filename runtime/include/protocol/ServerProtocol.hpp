/*
 * @file runtime/include/protocol/ServerProtocol.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Runtime public interfaces for protocol, command, client, and FFI boundaries.
 */

#ifndef BLITZAR_RUNTIME_INCLUDE_PROTOCOL_SERVERPROTOCOL_HPP_
#define BLITZAR_RUNTIME_INCLUDE_PROTOCOL_SERVERPROTOCOL_HPP_
#include <cstdint>
#include <string_view>

namespace bltzr_protocol {
extern const std::string_view SchemaVersion;
extern const std::string_view Status;
extern const std::string_view GetSnapshot;
extern const std::string_view Pause;
extern const std::string_view Resume;
extern const std::string_view Toggle;
extern const std::string_view Reset;
extern const std::string_view Recover;
extern const std::string_view Step;
extern const std::string_view SetDt;
extern const std::string_view SetSolver;
extern const std::string_view SetIntegrator;
extern const std::string_view SetPerformanceProfile;
extern const std::string_view SetParticleCount;
extern const std::string_view SetSph;
extern const std::string_view SetOctree;
extern const std::string_view SetSphParams;
extern const std::string_view SetSubsteps;
extern const std::string_view SetEnergyMeasure;
extern const std::string_view SetGpuTelemetry;
extern const std::string_view SetSnapshotPublishCadence;
extern const std::string_view SetSnapshotTransferCap;
extern const std::string_view Load;
extern const std::string_view Export;
extern const std::string_view SaveCheckpoint;
extern const std::string_view LoadCheckpoint;
extern const std::string_view Shutdown;
inline constexpr std::uint32_t kSnapshotMinPoints = 1u;
inline constexpr std::uint32_t kSnapshotDefaultPoints = 4096u;
inline constexpr std::uint32_t kSnapshotMaxPoints = 20000u;
std::uint32_t clampSnapshotPoints(std::uint32_t requested);
} // namespace bltzr_protocol
#endif // BLITZAR_RUNTIME_INCLUDE_PROTOCOL_SERVERPROTOCOL_HPP_
