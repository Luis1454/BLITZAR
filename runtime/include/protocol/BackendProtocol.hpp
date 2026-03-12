#ifndef GRAVITY_RUNTIME_INCLUDE_PROTOCOL_BACKENDPROTOCOL_HPP_
#define GRAVITY_RUNTIME_INCLUDE_PROTOCOL_BACKENDPROTOCOL_HPP_

#include <cstdint>
#include <string_view>

namespace grav_protocol {
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
extern const std::string_view SetParticleCount;
extern const std::string_view SetSph;
extern const std::string_view SetOctree;
extern const std::string_view SetSphParams;
extern const std::string_view SetEnergyMeasure;
extern const std::string_view Load;
extern const std::string_view Export;
extern const std::string_view Shutdown;

inline constexpr std::uint32_t kSnapshotMinPoints = 1u;
inline constexpr std::uint32_t kSnapshotDefaultPoints = 4096u;
inline constexpr std::uint32_t kSnapshotMaxPoints = 20000u;

std::uint32_t clampSnapshotPoints(std::uint32_t requested);

} // namespace grav_protocol


#endif // GRAVITY_RUNTIME_INCLUDE_PROTOCOL_BACKENDPROTOCOL_HPP_
