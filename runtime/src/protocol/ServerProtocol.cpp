#include "protocol/ServerProtocol.hpp"

namespace grav_protocol {
const std::string_view SchemaVersion = "server-json-v1";
const std::string_view Status = "status";
const std::string_view GetSnapshot = "get_snapshot";
const std::string_view Pause = "pause";
const std::string_view Resume = "resume";
const std::string_view Toggle = "toggle";
const std::string_view Reset = "reset";
const std::string_view Recover = "recover";
const std::string_view Step = "step";
const std::string_view SetDt = "set_dt";
const std::string_view SetSolver = "set_solver";
const std::string_view SetIntegrator = "set_integrator";
const std::string_view SetPerformanceProfile = "set_performance_profile";
const std::string_view SetParticleCount = "set_particle_count";
const std::string_view SetSph = "set_sph";
const std::string_view SetOctree = "set_octree";
const std::string_view SetSphParams = "set_sph_params";
const std::string_view SetSubsteps = "set_substeps";
const std::string_view SetEnergyMeasure = "set_energy_measure";
const std::string_view SetSnapshotPublishCadence = "set_snapshot_publish_cadence";
const std::string_view SetSnapshotTransferCap = "set_snapshot_transfer_cap";
const std::string_view Load = "load";
const std::string_view Export = "export";
const std::string_view Shutdown = "shutdown";

std::uint32_t clampSnapshotPoints(std::uint32_t requested)
{
    if (requested < kSnapshotMinPoints) {
        return kSnapshotMinPoints;
    }
    if (requested > kSnapshotMaxPoints) {
        return kSnapshotMaxPoints;
    }
    return requested;
}

} // namespace grav_protocol
