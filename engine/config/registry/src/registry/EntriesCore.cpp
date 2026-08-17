/*
 * @file engine/config/registry/src/registry/EntriesCore.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Responsibility-focused simulation option definitions.
 */

#include "Internal.hpp"

#include "core/Config.hpp"

#include <cstddef>

namespace bltzr_config {

extern const SimulationOptionEntry kCorePrimaryOptions[] = {
    {SimulationOptionGroup::Core, OptionKind::Uint, "--particle-count", "", "particle_count", "",
     "BLITZAR_SERVER_PARTICLES", "  --particle-count <n>\n", "",
     offsetof(SimulationConfig, particleCount), 2.0, 0.0, true, false},
    {SimulationOptionGroup::Core, OptionKind::Float, "--dt", "", "dt", "", "", "  --dt <float>\n",
     "", offsetof(SimulationConfig, dt), kMinSimulationDt, 0.0, true, false},
    {SimulationOptionGroup::Core, OptionKind::Solver, "--solver", "", "solver", "", "",
     "  --solver <pairwise_cuda|octree_gpu|octree_cpu|fmm_cpu>\n", "",
     offsetof(SimulationConfig, solver), 0.0, 0.0, false, false},
    {SimulationOptionGroup::Core, OptionKind::Integrator, "--integrator", "", "integrator", "", "",
     "  --integrator <euler|rk4|leapfrog>\n", "", offsetof(SimulationConfig, integrator), 0.0, 0.0,
     false, false},
    {SimulationOptionGroup::Core, OptionKind::PerformanceProfile, "--performance-profile", "",
     "performance_profile", "", "",
     "  --performance-profile <interactive|balanced|quality|custom>\n", "",
     offsetof(SimulationConfig, performanceProfile), 0.0, 0.0, false, false},
    {SimulationOptionGroup::Core, OptionKind::String, "--profile", "", "simulation_profile", "", "",
     "  --profile "
     "<disk_orbit|galaxy_collision|plummer_sphere|binary_star|solar_system|sph_collapse>\n",
     "", offsetof(SimulationConfig, simulationProfile), 0.0, 0.0, false, false},
    {SimulationOptionGroup::Core, OptionKind::Float, "--substep-target-dt", "", "substep_target_dt",
     "", "", "  --substep-target-dt <float|0=auto>\n", "",
     offsetof(SimulationConfig, substepTargetDt), 0.0, 0.0, true, false},
    {SimulationOptionGroup::Core, OptionKind::Uint, "--max-substeps", "", "max_substeps", "", "",
     "  --max-substeps <n>\n", "", offsetof(SimulationConfig, maxSubsteps), 1.0, 1024.0, true,
     true},
    {SimulationOptionGroup::Core, OptionKind::Bool, "--adaptive-time-steps", "",
     "adaptive_time_steps", "", "", "  --adaptive-time-steps <true|false>\n", "",
     offsetof(SimulationConfig, adaptiveTimeStepsEnabled), 0.0, 0.0, false, false},
    {SimulationOptionGroup::Core, OptionKind::Bool, "--adaptive-cost-guard", "",
     "adaptive_cost_guard", "", "", "  --adaptive-cost-guard <true|false>\n", "",
     offsetof(SimulationConfig, adaptiveTimeStepCostGuard), 0.0, 0.0, false, false},
    {SimulationOptionGroup::Core, OptionKind::Uint, "--adaptive-max-level", "",
     "adaptive_max_level", "", "", "  --adaptive-max-level <0..12>\n", "",
     offsetof(SimulationConfig, adaptiveTimeStepMaxLevel), 0.0, 12.0, true, true},
    {SimulationOptionGroup::Core, OptionKind::Float, "--adaptive-eta", "", "adaptive_eta", "", "",
     "  --adaptive-eta <0.01..1>\n", "", offsetof(SimulationConfig, adaptiveTimeStepEta), 0.01, 1.0,
     true, true},
    {SimulationOptionGroup::Core, OptionKind::Uint, "--snapshot-publish-ms", "",
     "snapshot_publish_period_ms", "", "", "  --snapshot-publish-ms <n>\n", "",
     offsetof(SimulationConfig, snapshotPublishPeriodMs), 1.0, 1000.0, true, true},
    {SimulationOptionGroup::Core, OptionKind::Float, "--octree-theta", "", "octree_theta", "", "",
     "  --octree-theta <float>\n", "", offsetof(SimulationConfig, octreeTheta), 0.01, 0.0, true,
     false},
    {SimulationOptionGroup::Core, OptionKind::Float, "--octree-softening", "", "octree_softening",
     "", "", "  --octree-softening <float>\n", "", offsetof(SimulationConfig, octreeSoftening),
     0.000001, 0.0, true, false},
    {SimulationOptionGroup::Core, OptionKind::OctreeCriterion, "--octree-opening-criterion", "",
     "octree_opening_criterion", "", "", "  --octree-opening-criterion <com|bounds>\n", "",
     offsetof(SimulationConfig, octreeOpeningCriterion), 0.0, 0.0, false, false},
    {SimulationOptionGroup::Core, OptionKind::Bool, "--octree-theta-auto-tune", "",
     "octree_theta_auto_tune", "", "", "  --octree-theta-auto-tune <true|false>\n", "",
     offsetof(SimulationConfig, octreeThetaAutoTune), 0.0, 0.0, false, false},
    {SimulationOptionGroup::Core, OptionKind::Float, "--octree-theta-auto-min", "",
     "octree_theta_auto_min", "", "", "  --octree-theta-auto-min <float>\n", "",
     offsetof(SimulationConfig, octreeThetaAutoMin), 0.01, 4.0, true, true},
    {SimulationOptionGroup::Core, OptionKind::Float, "--octree-theta-auto-max", "",
     "octree_theta_auto_max", "", "", "  --octree-theta-auto-max <float>\n", "",
     offsetof(SimulationConfig, octreeThetaAutoMax), 0.01, 4.0, true, true},
    {SimulationOptionGroup::Core, OptionKind::Bool, "--treepm-enabled", "", "treepm_enabled", "",
     "", "  --treepm-enabled <true|false>\n", "", offsetof(SimulationConfig, treePmEnabled), 0.0,
     0.0, false, false},
    {SimulationOptionGroup::Core, OptionKind::TreePmPreset, "--treepm-preset", "", "treepm_preset",
     "", "",
     "  --treepm-preset <pm_only|local_grid_fast|hybrid_balanced|"
     "hybrid_quality|tree_quality|custom>\n",
     "", offsetof(SimulationConfig, treePmPreset), 0.0, 0.0, false, false},
    {SimulationOptionGroup::Core, OptionKind::TreePmModel, "--treepm-model", "", "treepm_model", "",
     "", "  --treepm-model <auto|local_grid|tree|exact_tree|hybrid|pm_only>\n", "",
     offsetof(SimulationConfig, treePmModel), 0.0, 0.0, false, false},
    {SimulationOptionGroup::Core, OptionKind::TreePmLayout, "--treepm-layout", "", "treepm_layout",
     "", "", "  --treepm-layout <auto|linear|gather_linear|gather_morton>\n", "",
     offsetof(SimulationConfig, treePmLayout), 0.0, 0.0, false, false},
    {SimulationOptionGroup::Core, OptionKind::TreePmPrecision, "--treepm-precision", "",
     "treepm_precision", "", "", "  --treepm-precision <fp32|fp64>\n", "",
     offsetof(SimulationConfig, treePmPrecision), 0.0, 0.0, false, false},
    {SimulationOptionGroup::Core, OptionKind::TreePmAssignment, "--treepm-assignment", "",
     "treepm_assignment", "", "", "  --treepm-assignment <cic|tsc|pcs>\n", "",
     offsetof(SimulationConfig, treePmAssignment), 0.0, 0.0, false, false},
    {SimulationOptionGroup::Core, OptionKind::Bool, "--treepm-local-grid", "", "treepm_local_grid",
     "", "", "  --treepm-local-grid <true|false>\n", "",
     offsetof(SimulationConfig, treePmLocalGrid), 0.0, 0.0, false, false},
    {SimulationOptionGroup::Core, OptionKind::Uint, "--treepm-grid-size", "", "treepm_grid_size",
     "", "", "  --treepm-grid-size <32..128>\n", "", offsetof(SimulationConfig, treePmGridSize),
     32.0, 128.0, true, true},
    {SimulationOptionGroup::Core, OptionKind::Uint, "--treepm-jacobi-iters", "",
     "treepm_jacobi_iterations", "", "", "  --treepm-jacobi-iters <4..64>\n", "",
     offsetof(SimulationConfig, treePmJacobiIterations), 4.0, 64.0, true, true},
    {SimulationOptionGroup::Core, OptionKind::Float, "--treepm-cutoff-factor", "",
     "treepm_cutoff_factor", "", "", "  --treepm-cutoff-factor <1..2>\n", "",
     offsetof(SimulationConfig, treePmCutoffFactor), 1.0, 2.0, true, true},
    {SimulationOptionGroup::Core, OptionKind::Uint, "--treepm-max-local-neighbors", "",
     "treepm_max_local_neighbors", "", "", "  --treepm-max-local-neighbors <0..256>\n", "",
     offsetof(SimulationConfig, treePmMaxLocalNeighbors), 0.0, 256.0, true, true},
    {SimulationOptionGroup::Core, OptionKind::Uint, "--treepm-particle-limit", "",
     "treepm_particle_limit", "", "", "  --treepm-particle-limit <0|n>\n", "",
     offsetof(SimulationConfig, treePmParticleLimit), 0.0, 0.0, true, false},
    {SimulationOptionGroup::Core, OptionKind::Uint, "--treepm-dense-cell-threshold", "",
     "treepm_dense_cell_threshold", "", "", "  --treepm-dense-cell-threshold <1..>\n", "",
     offsetof(SimulationConfig, treePmDenseCellThreshold), 1.0, 0.0, true, false},
    {SimulationOptionGroup::Core, OptionKind::Bool, "--treepm-gravity-only-buffers", "",
     "treepm_gravity_only_buffers", "", "", "  --treepm-gravity-only-buffers <true|false>\n", "",
     offsetof(SimulationConfig, treePmGravityOnlyBuffers), 0.0, 0.0, false, false},
    {SimulationOptionGroup::Core, OptionKind::Uint, "--linear-octree-leaf-capacity", "",
     "linear_octree_leaf_capacity", "", "", "  --linear-octree-leaf-capacity <16..>\n", "",
     offsetof(SimulationConfig, linearOctreeLeafCapacity), 16.0, 0.0, true, false},
    {SimulationOptionGroup::Core, OptionKind::String, "--cuda-cache-preference", "",
     "cuda_cache_preference", "", "", "  --cuda-cache-preference <default|l1|shared>\n", "",
     offsetof(SimulationConfig, cudaCachePreference), 0.0, 0.0, false, false},
    {SimulationOptionGroup::Core, OptionKind::ClientParticleCap, "--client-particle-cap", "",
     "client_particle_cap", "", "BLITZAR_CLIENT_DRAW_CAP", "  --client-particle-cap <n>\n", "",
     offsetof(SimulationConfig, clientParticleCap), 0.0, 0.0, false, false},
};

extern const std::size_t kCorePrimaryOptionCount =
    sizeof(kCorePrimaryOptions) / sizeof(kCorePrimaryOptions[0]);

extern const SimulationOptionEntry kCoreTailOptions[] = {
    {SimulationOptionGroup::Core, OptionKind::Bool, "--deterministic", "", "deterministic_mode", "",
     "", "  --deterministic <true|false>\n", "", offsetof(SimulationConfig, deterministicMode), 0.0,
     0.0, false, false},
    {SimulationOptionGroup::Core, OptionKind::Float, "--physics-max-accel", "",
     "physics_max_acceleration", "", "", "  --physics-max-accel <float>\n", "",
     offsetof(SimulationConfig, physicsMaxAcceleration), 0.0, 0.0, true, false},
    {SimulationOptionGroup::Core, OptionKind::Float, "--physics-min-softening", "",
     "physics_min_softening", "", "", "  --physics-min-softening <float>\n", "",
     offsetof(SimulationConfig, physicsMinSoftening), 0.0, 0.0, true, false},
    {SimulationOptionGroup::Core, OptionKind::Float, "--physics-min-dist2", "",
     "physics_min_distance2", "", "", "  --physics-min-dist2 <float>\n", "",
     offsetof(SimulationConfig, physicsMinDistance2), 0.0, 0.0, true, false},
    {SimulationOptionGroup::Core, OptionKind::Float, "--physics-min-theta", "", "physics_min_theta",
     "", "", "  --physics-min-theta <float>\n", "", offsetof(SimulationConfig, physicsMinTheta),
     0.0, 0.0, true, false},
};

extern const std::size_t kCoreTailOptionCount =
    sizeof(kCoreTailOptions) / sizeof(kCoreTailOptions[0]);

} // namespace bltzr_config
