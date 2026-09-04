#include "solvers/treepm/TreePmSolver.hpp"

#include <cmath>
#include <new>
#include <stdexcept>
#include <utility>

namespace blitzar_treepm {

bool TreePmSettings::IsValid() const noexcept
{
    return std::isfinite(pm_weight) && pm_weight >= 0.0 && std::isfinite(tree_weight) &&
           tree_weight >= 0.0 && pm_weight + tree_weight > 0.0;
}

TreePmSolver::TreePmSolver(blitzar_pm::PmSolver pm, blitzar_barnes_hut::BhSolver tree,
    blitzar_solvers::SolverTreeResources& resources)
    : settings_{}, pm_(std::move(pm)), tree_(std::move(tree)), resources_(resources),
      staging_capacity_(pm_.Resource().MaxParticles()), pm_x_{}, pm_y_{}, pm_z_{}, tree_x_{},
      tree_y_{}, tree_z_{}
{
    tree_.BindResources(resources);

    if (staging_capacity_ != 0) {
        pm_x_.resize(staging_capacity_);
        pm_y_.resize(staging_capacity_);
        pm_z_.resize(staging_capacity_);
        tree_x_.resize(staging_capacity_);
        tree_y_.resize(staging_capacity_);
        tree_z_.resize(staging_capacity_);
    }
}

TreePmSolver::TreePmSolver(TreePmSolver&& other) noexcept
    : settings_(other.settings_), pm_(std::move(other.pm_)), tree_(std::move(other.tree_)),
      resources_(other.resources_), staging_capacity_(other.staging_capacity_),
      pm_x_(std::move(other.pm_x_)), pm_y_(std::move(other.pm_y_)), pm_z_(std::move(other.pm_z_)),
      tree_x_(std::move(other.tree_x_)), tree_y_(std::move(other.tree_y_)),
      tree_z_(std::move(other.tree_z_))
{
    tree_.BindResources(resources_.get());
}

TreePmSolver& TreePmSolver::operator=(TreePmSolver&& other) noexcept
{
    settings_ = other.settings_;
    pm_ = std::move(other.pm_);
    tree_ = std::move(other.tree_);
    resources_ = other.resources_;
    staging_capacity_ = other.staging_capacity_;
    pm_x_ = std::move(other.pm_x_);
    pm_y_ = std::move(other.pm_y_);
    pm_z_ = std::move(other.pm_z_);
    tree_x_ = std::move(other.tree_x_);
    tree_y_ = std::move(other.tree_y_);
    tree_z_ = std::move(other.tree_z_);
    tree_.BindResources(resources_.get());

    return *this;
}

void TreePmSolver::BindResources(blitzar_solvers::SolverTreeResources& resources) noexcept
{
    resources_ = resources;

    tree_.BindResources(resources);
}

blitzar_solvers::SolverKind TreePmSolver::Kind() const noexcept
{
    return blitzar_solvers::SolverKind::TreePm;
}

blitzar_status TreePmSolver::Prepare(std::size_t staging_capacity) noexcept
{
    const blitzar_status pm_status = pm_.Prepare(staging_capacity);

    if (pm_status != BLITZAR_STATUS_OK) {
        return pm_status;
    }

    const blitzar_status tree_status = tree_.Prepare(staging_capacity);

    if (tree_status != BLITZAR_STATUS_OK) {
        return tree_status;
    }

    return EnsureCapacity(staging_capacity);
}

blitzar_grid::GridResource& TreePmSolver::GridResource() noexcept
{
    return pm_.Resource();
}

const blitzar_grid::GridResource& TreePmSolver::GridResource() const noexcept
{
    return pm_.Resource();
}

blitzar_solvers::SolverTreeResources& TreePmSolver::Resources() noexcept
{
    return resources_.get();
}

const blitzar_solvers::SolverTreeResources& TreePmSolver::Resources() const noexcept
{
    return resources_.get();
}

blitzar_status TreePmSolver::EnsureCapacity(std::size_t staging_capacity) noexcept
{
    if (staging_capacity <= staging_capacity_) {
        return BLITZAR_STATUS_OK;
    }

    try {
        pm_x_.resize(staging_capacity);
        pm_y_.resize(staging_capacity);
        pm_z_.resize(staging_capacity);
        tree_x_.resize(staging_capacity);
        tree_y_.resize(staging_capacity);
        tree_z_.resize(staging_capacity);
    }
    catch (const std::length_error&) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }

    staging_capacity_ = staging_capacity;

    return BLITZAR_STATUS_OK;
}

} // namespace blitzar_treepm
