namespace blitzar_hip {

namespace {

[[nodiscard]] blitzar_status HipStatus(hipError_t error) noexcept
{
    return error == hipErrorMemoryAllocation ? BLITZAR_STATUS_ALLOCATION_FAILURE
                                             : BLITZAR_STATUS_INTERNAL_ERROR;
}

[[nodiscard]] blitzar_status FaultStatus(Fault fault) noexcept
{
    switch (fault) {
    case Fault::None:

        return BLITZAR_STATUS_OK;

    case Fault::AllocationFailure:

        return BLITZAR_STATUS_ALLOCATION_FAILURE;

    case Fault::LaunchFailure:
    case Fault::SynchronizationFailure:

        return BLITZAR_STATUS_INTERNAL_ERROR;

    case Fault::NonFiniteResult:

        return BLITZAR_STATUS_INVALID_ARGUMENT;

    default:

        return BLITZAR_STATUS_INTERNAL_ERROR;
    }
}

[[nodiscard]] bool SameSettings(const blitzar_barnes_hut::BarnesHutSettings& left,
    const blitzar_barnes_hut::BarnesHutSettings& right) noexcept
{
    return left.opening_angle == right.opening_angle && left.max_particles == right.max_particles &&
           left.max_cells == right.max_cells && left.leaf_capacity == right.leaf_capacity &&
           left.max_depth == right.max_depth;
}

[[nodiscard]] bool ValidDirectInput(blitzar_core::ParticleStateView particles,
    blitzar_core::ForceView forces, blitzar_physics::GravityParameters gravity,
    blitzar_solvers::ForceRange range) noexcept
{
    return blitzar_core::IsValid(particles) && blitzar_core::IsValid(forces) &&
           particles.count == forces.count && gravity.IsValid() &&
           range.IsValid(particles.SourceCount());
}

[[nodiscard]] bool ValidBarnesInput(const BarnesHutComputeRequest& request) noexcept
{
    return blitzar_core::IsValid(request.particles) && blitzar_core::IsValid(request.forces) &&
           request.particles.count == request.forces.count && request.execution.IsValid() &&
           request.gravity.IsValid() && request.settings.IsValid() &&
           request.particles.SourceCount() <= request.settings.max_particles;
}

} // namespace

} // namespace blitzar_hip
