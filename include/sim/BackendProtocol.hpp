#ifndef GRAVITY_SIM_BACKENDPROTOCOL_HPP
#define GRAVITY_SIM_BACKENDPROTOCOL_HPP

#include <cstdint>
#include <string_view>

namespace sim::protocol {

namespace cmd {
inline constexpr std::string_view Status = "status";
inline constexpr std::string_view GetSnapshot = "get_snapshot";
inline constexpr std::string_view Pause = "pause";
inline constexpr std::string_view Resume = "resume";
inline constexpr std::string_view Toggle = "toggle";
inline constexpr std::string_view Reset = "reset";
inline constexpr std::string_view Recover = "recover";
inline constexpr std::string_view Step = "step";
inline constexpr std::string_view SetDt = "set_dt";
inline constexpr std::string_view SetSolver = "set_solver";
inline constexpr std::string_view SetIntegrator = "set_integrator";
inline constexpr std::string_view SetParticleCount = "set_particle_count";
inline constexpr std::string_view SetSph = "set_sph";
inline constexpr std::string_view SetOctree = "set_octree";
inline constexpr std::string_view SetSphParams = "set_sph_params";
inline constexpr std::string_view SetEnergyMeasure = "set_energy_measure";
inline constexpr std::string_view Load = "load";
inline constexpr std::string_view Export = "export";
inline constexpr std::string_view Shutdown = "shutdown";
} // namespace cmd

constexpr std::uint32_t kSnapshotMinPoints = 1u;
constexpr std::uint32_t kSnapshotDefaultPoints = 4096u;
constexpr std::uint32_t kSnapshotMaxPoints = 20000u;

inline std::uint32_t clampSnapshotPoints(std::uint32_t requested)
{
    if (requested < kSnapshotMinPoints) {
        return kSnapshotMinPoints;
    }
    if (requested > kSnapshotMaxPoints) {
        return kSnapshotMaxPoints;
    }
    return requested;
}

} // namespace sim::protocol

#endif // GRAVITY_SIM_BACKENDPROTOCOL_HPP
