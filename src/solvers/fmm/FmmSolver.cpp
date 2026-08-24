#include "solvers/fmm/FmmSolver.hpp"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <limits>
#include <new>
#include <stdexcept>

namespace blitzar_fmm {

namespace {

[[nodiscard]] blitzar_status PairStatusToStatus(
    blitzar_physics::PairStatus status) noexcept
{
    if (status == blitzar_physics::PairStatus::Singularity) {
        return BLITZAR_STATUS_SINGULARITY;
    }
    if (status == blitzar_physics::PairStatus::Invalid) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    return BLITZAR_STATUS_OK;
}

} // namespace

FmmSolver::FmmSolver(blitzar_physics::GravityParameters gravity, FmmSettings settings,
    std::size_t local_particle_capacity)
    : settings_(settings), parameters_(gravity), gravity_(gravity),
      local_particle_capacity_(local_particle_capacity == 0 ? settings.max_particles
                                                             : local_particle_capacity),
      local_cell_capacity_(LocalCellCapacity(settings.max_cells,
          local_particle_capacity == 0 ? settings.max_particles : local_particle_capacity)),
      tree_(std::make_unique<blitzar_trees::Octree>(local_particle_capacity_, local_cell_capacity_,
          settings.leaf_capacity, settings.max_depth)),
      remote_tree_(std::make_unique<blitzar_trees::Octree>(settings.max_particles,
          settings.max_cells, settings.leaf_capacity, settings.max_depth)),
      stack_pool_(settings.max_cells, settings.max_depth), multipoles_{}, remote_multipoles_{},
      staging_(local_particle_capacity_)
{
    multipoles_.reserve(local_cell_capacity_);
    remote_multipoles_.reserve(settings.max_cells);
}

blitzar_core::SolverKind FmmSolver::Kind() const noexcept
{
    return blitzar_core::SolverKind::Fmm;
}

blitzar_status FmmSolver::Prepare(std::size_t particle_capacity) noexcept
{
    return EnsureLocalCapacity(particle_capacity);
}

std::size_t FmmSolver::BuildCount() const noexcept
{
    return tree_ == nullptr ? 0 : tree_->BuildCount();
}

std::size_t FmmSolver::RefitCount() const noexcept
{
    return tree_ == nullptr ? 0 : tree_->RefitCount();
}

std::size_t FmmSolver::LocalCellCapacity(
    std::size_t configured_cells, std::size_t particle_capacity) noexcept
{
    if (particle_capacity == 0 || configured_cells == 0) {
        return configured_cells;
    }

    const std::size_t maximum = std::numeric_limits<std::size_t>::max();
    const std::size_t particle_cells =
        particle_capacity > (maximum - 1) / 8 ? maximum : particle_capacity * 8 + 1;

    return std::min(configured_cells, particle_cells);
}

bool FmmSolver::IsValidState(blitzar_core::ParticleStateView particles) noexcept
{
    if (!blitzar_core::IsValid(particles)) {
        return false;
    }

    for (std::size_t index = 0; index < particles.SourceCount(); ++index) {
        if (!std::isfinite(particles.x[index]) || !std::isfinite(particles.y[index]) ||
            !std::isfinite(particles.z[index]) || !std::isfinite(particles.velocity_x[index]) ||
            !std::isfinite(particles.velocity_y[index]) ||
            !std::isfinite(particles.velocity_z[index]) || !std::isfinite(particles.mass[index]) ||
            particles.mass[index] < 0.0) {
            return false;
        }
    }

    return true;
}

bool FmmSolver::Contains(
    const blitzar_trees::Octree::Cell& cell, blitzar_core::Vector3 position) noexcept
{
    return std::abs(position.x - cell.center.x) <= cell.half_extent &&
           std::abs(position.y - cell.center.y) <= cell.half_extent &&
           std::abs(position.z - cell.center.z) <= cell.half_extent;
}

blitzar_status FmmSolver::BuildMultipoles(const blitzar_trees::Octree& tree,
    blitzar_core::ParticleStateView sources, std::vector<Multipole>& multipoles) const noexcept
{
    const std::span<const blitzar_trees::Octree::Cell> cells = tree.Cells();

    if (cells.empty() || cells.size() > multipoles.capacity()) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    multipoles.resize(cells.size());

    for (std::size_t index = cells.size(); index-- > 0;) {
        const blitzar_trees::Octree::Cell& cell = cells[index];

        Multipole& multipole = multipoles[index];
        multipole = {};
        multipole.mass = cell.mass;

        if (cell.IsLeaf()) {
            for (std::size_t offset = 0; offset < cell.count; ++offset) {
                if (cell.begin > std::numeric_limits<std::size_t>::max() - offset) {
                    return BLITZAR_STATUS_INTERNAL_ERROR;
                }

                std::size_t particle = 0;

                if (!tree.ParticleIndex(cell.begin + offset, particle) ||
                    particle >= sources.SourceCount()) {
                    return BLITZAR_STATUS_INTERNAL_ERROR;
                }

                const blitzar_core::Scalar mass = sources.mass[particle];
                const blitzar_core::Scalar dx = sources.x[particle] - cell.center_of_mass.x;
                const blitzar_core::Scalar dy = sources.y[particle] - cell.center_of_mass.y;
                const blitzar_core::Scalar dz = sources.z[particle] - cell.center_of_mass.z;

                multipole.xx += mass * dx * dx;
                multipole.xy += mass * dx * dy;
                multipole.xz += mass * dx * dz;
                multipole.yy += mass * dy * dy;
                multipole.yz += mass * dy * dz;
                multipole.zz += mass * dz * dz;
            }
        }
        else {
            for (const std::size_t child : cell.children) {
                if (child == blitzar_trees::Octree::Cell::InvalidIndex) {
                    continue;
                }
                if (child >= cells.size()) {
                    return BLITZAR_STATUS_INTERNAL_ERROR;
                }

                const blitzar_trees::Octree::Cell& child_cell = cells[child];

                const Multipole& child_multipole = multipoles[child];

                const blitzar_core::Scalar dx =
                    child_cell.center_of_mass.x - cell.center_of_mass.x;

                const blitzar_core::Scalar dy =
                    child_cell.center_of_mass.y - cell.center_of_mass.y;

                const blitzar_core::Scalar dz =
                    child_cell.center_of_mass.z - cell.center_of_mass.z;

                multipole.xx += child_multipole.xx + child_multipole.mass * dx * dx;
                multipole.xy += child_multipole.xy + child_multipole.mass * dx * dy;
                multipole.xz += child_multipole.xz + child_multipole.mass * dx * dz;
                multipole.yy += child_multipole.yy + child_multipole.mass * dy * dy;
                multipole.yz += child_multipole.yz + child_multipole.mass * dy * dz;
                multipole.zz += child_multipole.zz + child_multipole.mass * dz * dz;
            }
        }

        if (!std::isfinite(multipole.mass) || !std::isfinite(multipole.xx) ||
            !std::isfinite(multipole.xy) || !std::isfinite(multipole.xz) ||
            !std::isfinite(multipole.yy) || !std::isfinite(multipole.yz) ||
            !std::isfinite(multipole.zz)) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }
    }

    return BLITZAR_STATUS_OK;
}

blitzar_status FmmSolver::EvaluateMultipole(const Multipole& multipole,
    blitzar_core::Vector3 displacement, blitzar_core::Scalar squared_distance,
    blitzar_core::Vector3& acceleration) const noexcept
{
    if (multipole.mass == 0.0) {
        return BLITZAR_STATUS_OK;
    }

    const blitzar_status pair_status = PairStatusToStatus(
        gravity_.ValidatePair(multipole.mass, squared_distance));

    if (pair_status != BLITZAR_STATUS_OK) {
        return pair_status;
    }

    const blitzar_core::Scalar softening = parameters_.EffectiveSoftening();
    const blitzar_core::Scalar softened_squared =
        squared_distance + softening * softening;

    const blitzar_core::Scalar inverse_squared = 1.0 / softened_squared;
    const blitzar_core::Scalar inverse_distance = 1.0 / std::sqrt(softened_squared);
    const blitzar_core::Scalar inverse_cubed = inverse_squared * inverse_distance;
    const blitzar_core::Scalar inverse_fifth = inverse_cubed * inverse_squared;
    const blitzar_core::Scalar inverse_seventh = inverse_fifth * inverse_squared;

    const blitzar_core::Vector3 moment_times_displacement{
        multipole.xx * displacement.x + multipole.xy * displacement.y +
            multipole.xz * displacement.z,
        multipole.xy * displacement.x + multipole.yy * displacement.y +
            multipole.yz * displacement.z,
        multipole.xz * displacement.x + multipole.yz * displacement.y +
            multipole.zz * displacement.z};

    const blitzar_core::Scalar trace = multipole.xx + multipole.yy + multipole.zz;
    const blitzar_core::Scalar contraction =
        displacement.x * moment_times_displacement.x +
        displacement.y * moment_times_displacement.y +
        displacement.z * moment_times_displacement.z;

    const blitzar_core::Scalar constant = parameters_.EffectiveConstant();

    acceleration.x += constant *
        (multipole.mass * displacement.x * inverse_cubed -
            3.0 * moment_times_displacement.x * inverse_fifth -
            1.5 * displacement.x * trace * inverse_fifth +
            7.5 * displacement.x * contraction * inverse_seventh);

    acceleration.y += constant *
        (multipole.mass * displacement.y * inverse_cubed -
            3.0 * moment_times_displacement.y * inverse_fifth -
            1.5 * displacement.y * trace * inverse_fifth +
            7.5 * displacement.y * contraction * inverse_seventh);

    acceleration.z += constant *
        (multipole.mass * displacement.z * inverse_cubed -
            3.0 * moment_times_displacement.z * inverse_fifth -
            1.5 * displacement.z * trace * inverse_fifth +
            7.5 * displacement.z * contraction * inverse_seventh);

    return std::isfinite(acceleration.x) && std::isfinite(acceleration.y) &&
                   std::isfinite(acceleration.z)
               ? BLITZAR_STATUS_OK
               : BLITZAR_STATUS_INVALID_ARGUMENT;
}

blitzar_status FmmSolver::Accumulate(const AccumulationRequest& request) const noexcept
{
    if (request.stack.empty()) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    std::size_t stack_size = 1;

    request.stack[0] = 0;
    request.acceleration = {};

    const blitzar_core::Vector3 target_position{
        request.targets.x[request.target], request.targets.y[request.target],
        request.targets.z[request.target]};

    while (stack_size > 0) {
        const std::size_t cell_index = request.stack[--stack_size];

        if (cell_index >= request.multipoles.size()) {
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }

        const std::span<const blitzar_trees::Octree::Cell> cell_view =
            request.tree.CellAt(cell_index);

        if (cell_view.empty()) {
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }

        const blitzar_trees::Octree::Cell& cell = cell_view.front();

        const Multipole& multipole = request.multipoles[cell_index];

        if (multipole.mass == 0.0) {
            continue;
        }

        const bool contains_target = Contains(cell, target_position);

        if (cell.IsLeaf()) {
            for (std::size_t offset = 0; offset < cell.count; ++offset) {
                if (cell.begin > std::numeric_limits<std::size_t>::max() - offset) {
                    return BLITZAR_STATUS_INTERNAL_ERROR;
                }

                std::size_t source = 0;

                if (!request.tree.ParticleIndex(cell.begin + offset, source) ||
                    source >= request.sources.SourceCount()) {
                    return BLITZAR_STATUS_INTERNAL_ERROR;
                }
                if ((request.skip_self && source == request.target) ||
                    request.sources.mass[source] == 0.0) {
                    continue;
                }

                const blitzar_core::Scalar dx = request.sources.x[source] - target_position.x;
                const blitzar_core::Scalar dy = request.sources.y[source] - target_position.y;
                const blitzar_core::Scalar dz = request.sources.z[source] - target_position.z;
                const blitzar_core::Scalar squared_distance = dx * dx + dy * dy + dz * dz;
                const blitzar_status pair_status = PairStatusToStatus(
                    gravity_.ValidatePair(request.sources.mass[source], squared_distance));

                if (pair_status != BLITZAR_STATUS_OK) {
                    return pair_status;
                }

                const blitzar_core::Scalar factor =
                    gravity_.PairFactor(request.sources.mass[source], squared_distance);

                request.acceleration.x += factor * dx;
                request.acceleration.y += factor * dy;
                request.acceleration.z += factor * dz;
            }

            continue;
        }

        const blitzar_core::Scalar dx = cell.center_of_mass.x - target_position.x;
        const blitzar_core::Scalar dy = cell.center_of_mass.y - target_position.y;
        const blitzar_core::Scalar dz = cell.center_of_mass.z - target_position.z;
        const blitzar_core::Scalar squared_distance = dx * dx + dy * dy + dz * dz;
        const blitzar_core::Scalar distance = std::sqrt(squared_distance);
        const blitzar_core::Scalar cell_width = 2.0 * cell.half_extent;
        const bool well_separated = !contains_target && distance > 0.0 &&
                                     cell_width / distance < settings_.opening_angle;

        if (well_separated) {
            const blitzar_status multipole_status = EvaluateMultipole(
                multipole, {dx, dy, dz}, squared_distance, request.acceleration);

            if (multipole_status != BLITZAR_STATUS_OK) {
                return multipole_status;
            }

            continue;
        }

        for (auto child = cell.children.rbegin(); child != cell.children.rend(); ++child) {
            if (*child == blitzar_trees::Octree::Cell::InvalidIndex) {
                continue;
            }
            if (*child >= request.multipoles.size() || stack_size == request.stack.size()) {
                return BLITZAR_STATUS_INTERNAL_ERROR;
            }

            request.stack[stack_size++] = *child;
        }
    }

    return std::isfinite(request.acceleration.x) && std::isfinite(request.acceleration.y) &&
                   std::isfinite(request.acceleration.z)
               ? BLITZAR_STATUS_OK
               : BLITZAR_STATUS_INVALID_ARGUMENT;
}

blitzar_status FmmSolver::Compute(blitzar_core::ParticleStateView particles,
    blitzar_core::ForceView forces, const blitzar_core::ExecutionSettings& settings) noexcept
{
    return Compute(particles, forces, settings, stack_pool_);
}

blitzar_status FmmSolver::Compute(blitzar_core::ParticleStateView particles,
    blitzar_core::ForceView forces, const blitzar_core::ExecutionSettings& settings,
    blitzar_barnes_hut::ThreadStackPool& stack_pool) noexcept
{
    const blitzar_status prepare_status = Prepare(particles.count);

    if (prepare_status != BLITZAR_STATUS_OK) {
        return prepare_status;
    }

    const blitzar_status status = ComputeTree(
        {*tree_, multipoles_, particles, particles, forces, settings, stack_pool, false, true});

    return status == BLITZAR_STATUS_OK ? CommitStagedForces(forces) : status;
}

blitzar_status FmmSolver::ComputeSplit(const FmmSplitRequest& request) noexcept
{
    return ComputeSplit(request, stack_pool_);
}

blitzar_status FmmSolver::ComputeSplit(
    const FmmSplitRequest& request, blitzar_barnes_hut::ThreadStackPool& stack_pool) noexcept
{
    const blitzar_status prepare_status = Prepare(request.local.count);

    if (prepare_status != BLITZAR_STATUS_OK) {
        return prepare_status;
    }

    const blitzar_status local_status = ComputeTree(
        {*tree_, multipoles_, request.local, request.local, request.forces, request.settings,
            stack_pool, false, true});

    if (local_status != BLITZAR_STATUS_OK || request.remote.SourceCount() == 0) {
        return local_status == BLITZAR_STATUS_OK ? CommitStagedForces(request.forces) : local_status;
    }

    const blitzar_status remote_status = ComputeTree(
        {*remote_tree_, remote_multipoles_, request.local, request.remote, request.forces,
            request.settings, stack_pool, true, false});

    return remote_status == BLITZAR_STATUS_OK ? CommitStagedForces(request.forces) : remote_status;
}

blitzar_status FmmSolver::ComputeTree(const TreeComputeRequest& request) noexcept
{
    const bool local_tree = tree_ != nullptr && &request.tree == tree_.get();
    const bool remote_tree = remote_tree_ != nullptr && &request.tree == remote_tree_.get();
    const std::size_t source_capacity = local_tree ? local_particle_capacity_ : settings_.max_particles;

    if ((!local_tree && !remote_tree) || !settings_.IsValid() || !gravity_.IsValid() ||
        !request.settings.IsValid() || request.targets.count != request.forces.count ||
        request.sources.SourceCount() > source_capacity || !IsValidState(request.targets) ||
        !IsValidState(request.sources) || !blitzar_core::IsValid(request.forces) ||
        request.stack_pool.MaxCells() < settings_.max_cells ||
        request.stack_pool.MaxDepth() < settings_.max_depth ||
        staging_.size() < request.targets.count) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    if (request.sources.SourceCount() == 0) {
        if (!request.accumulate) {
            for (std::size_t index = 0; index < request.targets.count; ++index) {
                staging_[index] = {};
            }
        }

        return BLITZAR_STATUS_OK;
    }

    if (!request.tree.Refit(request.sources)) {
        const blitzar_status build_status = request.tree.Build(request.sources);

        if (build_status != BLITZAR_STATUS_OK) {
            return build_status;
        }
    }

    const blitzar_status multipole_status =
        BuildMultipoles(request.tree, request.sources, request.multipoles);

    if (multipole_status != BLITZAR_STATUS_OK) {
        return multipole_status;
    }

    std::atomic<blitzar_status> computation_status{BLITZAR_STATUS_OK};

#if defined(_OPENMP)
#pragma omp parallel for schedule(static)
#endif

    for (std::int64_t target_index = 0;
         target_index < static_cast<std::int64_t>(request.targets.count); ++target_index) {
        if (computation_status.load(std::memory_order_relaxed) != BLITZAR_STATUS_OK) {
            continue;
        }

        const std::size_t target = static_cast<std::size_t>(target_index);
        blitzar_core::Vector3 acceleration{};
        const AccumulationRequest accumulation{request.tree, request.multipoles,
            request.targets, request.sources, target,
            request.stack_pool.Stack(blitzar_barnes_hut::ThreadStackPool::CurrentThread()),
            acceleration, request.skip_self};

        const blitzar_status target_status = Accumulate(accumulation);

        if (target_status != BLITZAR_STATUS_OK) {
            blitzar_status expected = BLITZAR_STATUS_OK;

            computation_status.compare_exchange_strong(
                expected, target_status, std::memory_order_relaxed, std::memory_order_relaxed);

            continue;
        }

        if (request.accumulate) {
            staging_[target].x += acceleration.x;
            staging_[target].y += acceleration.y;
            staging_[target].z += acceleration.z;
        }
        else {
            staging_[target] = acceleration;
        }
    }

    return computation_status.load(std::memory_order_relaxed);
}

blitzar_status FmmSolver::CommitStagedForces(blitzar_core::ForceView forces) noexcept
{
    if (!blitzar_core::IsValid(forces) || forces.count > staging_.size()) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    for (std::size_t index = 0; index < forces.count; ++index) {
        forces.x[index] = staging_[index].x;
        forces.y[index] = staging_[index].y;
        forces.z[index] = staging_[index].z;
    }

    return BLITZAR_STATUS_OK;
}

blitzar_status FmmSolver::EnsureLocalCapacity(std::size_t particle_capacity) noexcept
{
    if (particle_capacity <= local_particle_capacity_) {
        return BLITZAR_STATUS_OK;
    }
    if (!settings_.IsValid() || particle_capacity > settings_.max_particles) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const std::size_t cell_capacity = LocalCellCapacity(settings_.max_cells, particle_capacity);
    std::unique_ptr<blitzar_trees::Octree> candidate_tree;
    std::vector<Multipole> candidate_multipoles;
    std::vector<blitzar_core::Vector3> candidate_staging;

    try {
        candidate_tree = std::make_unique<blitzar_trees::Octree>(particle_capacity, cell_capacity,
            settings_.leaf_capacity, settings_.max_depth);

        candidate_multipoles.reserve(cell_capacity);
        candidate_staging.resize(particle_capacity);
    }
    catch (const std::length_error&) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }

    tree_ = std::move(candidate_tree);
    multipoles_ = std::move(candidate_multipoles);
    staging_ = std::move(candidate_staging);
    local_particle_capacity_ = particle_capacity;
    local_cell_capacity_ = cell_capacity;

    return BLITZAR_STATUS_OK;
}

} // namespace blitzar_fmm
