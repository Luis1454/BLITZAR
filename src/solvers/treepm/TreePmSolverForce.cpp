#include "solvers/treepm/TreePmSolver.hpp"

#include <cmath>
#include <span>

namespace blitzar_treepm {

bool TreePmSolver::IsValidState(blitzar_core::ParticleStateView particles) noexcept
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

bool TreePmSolver::ValidateRequest(
    const blitzar_solvers::SolverForceRequest::TreePm& request) const noexcept
{
    return settings_.IsValid() &&
           request.source_kind == blitzar_solvers::SolverForceSourceKind::Local &&
           blitzar_core::IsValid(request.targets) && blitzar_core::IsValid(request.sources) &&
           blitzar_core::IsValid(request.forces) && request.targets.count == request.forces.count &&
           request.settings.IsValid() && IsValidState(request.targets) &&
           IsValidState(request.sources) && request.targets.count <= staging_capacity_ &&
           request.sources.SourceCount() <= pm_.Resource().MaxParticles() &&
           &request.grid_resource == &pm_.Resource() && pm_.Resource().IsCurrent(request.grid) &&
           &request.tree_resource == &resources_.get().Local() &&
           resources_.get().Local().IsCurrent(request.tree);
}

blitzar_status TreePmSolver::Evaluate(
    const blitzar_solvers::SolverForceRequest::TreePm& request) noexcept
{
    if (!ValidateRequest(request)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const blitzar_status component_status = EvaluateComponents(request);

    return component_status == BLITZAR_STATUS_OK ? Commit(request) : component_status;
}

blitzar_status TreePmSolver::EvaluateComponents(
    const blitzar_solvers::SolverForceRequest::TreePm& request) noexcept
{
    const blitzar_core::ForceView pm_forces{request.targets.count,
        std::span<blitzar_core::Scalar>(pm_x_).first(request.targets.count),
        std::span<blitzar_core::Scalar>(pm_y_).first(request.targets.count),
        std::span<blitzar_core::Scalar>(pm_z_).first(request.targets.count)};

    const blitzar_solvers::SolverForceRequest::Grid pm_request{request.targets, request.sources,
        pm_forces, request.settings, request.grid_resource, request.grid,
        blitzar_solvers::SolverForceSourceKind::Local, false, request.skip_self};

    const blitzar_status pm_status = pm_.Evaluate(pm_request);

    if (pm_status != BLITZAR_STATUS_OK) {
        return pm_status;
    }

    const blitzar_core::ForceView tree_forces{request.targets.count,
        std::span<blitzar_core::Scalar>(tree_x_).first(request.targets.count),
        std::span<blitzar_core::Scalar>(tree_y_).first(request.targets.count),
        std::span<blitzar_core::Scalar>(tree_z_).first(request.targets.count)};

    const blitzar_solvers::SolverForceRequest::Tree tree_request{request.targets, request.sources,
        tree_forces, request.settings, request.tree_resource, request.tree,
        blitzar_solvers::SolverForceSourceKind::Local, false, request.skip_self};

    return tree_.Evaluate(tree_request);
}

blitzar_status TreePmSolver::Commit(
    const blitzar_solvers::SolverForceRequest::TreePm& request) noexcept
{
    if (!blitzar_core::IsValid(request.forces) || request.forces.count > staging_capacity_) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    for (std::size_t target = 0; target < request.forces.count; ++target) {
        const blitzar_core::Scalar x =
            settings_.pm_weight * pm_x_[target] + settings_.tree_weight * tree_x_[target];

        const blitzar_core::Scalar y =
            settings_.pm_weight * pm_y_[target] + settings_.tree_weight * tree_y_[target];

        const blitzar_core::Scalar z =
            settings_.pm_weight * pm_z_[target] + settings_.tree_weight * tree_z_[target];

        if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        if (request.accumulate) {
            request.forces.x[target] += x;
            request.forces.y[target] += y;
            request.forces.z[target] += z;
        }
        else {
            request.forces.x[target] = x;
            request.forces.y[target] = y;
            request.forces.z[target] = z;
        }
    }

    return BLITZAR_STATUS_OK;
}

} // namespace blitzar_treepm
