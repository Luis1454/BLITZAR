#ifndef BLITZAR_BLITZAR_HPP
#define BLITZAR_BLITZAR_HPP

#include <atomic>
#include <blitzar/blitzar.h>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <span>

namespace blitzar {

[[nodiscard]] inline const char* version() noexcept
{
    return blitzar_version();
}

[[nodiscard]] inline const char* plan_version() noexcept
{
    return blitzar_plan_version();
}

enum class Status : std::int32_t {
    Ok = BLITZAR_STATUS_OK,
    InvalidArgument = BLITZAR_STATUS_INVALID_ARGUMENT,
    AllocationFailure = BLITZAR_STATUS_ALLOCATION_FAILURE,
    InternalError = BLITZAR_STATUS_INTERNAL_ERROR,
    Singularity = BLITZAR_STATUS_SINGULARITY,
    Unsupported = BLITZAR_STATUS_UNSUPPORTED
};

[[nodiscard]] constexpr Status FromCStatus(blitzar_status status) noexcept
{
    switch (status) {
    case BLITZAR_STATUS_OK:

        return Status::Ok;

    case BLITZAR_STATUS_INVALID_ARGUMENT:

        return Status::InvalidArgument;

    case BLITZAR_STATUS_ALLOCATION_FAILURE:

        return Status::AllocationFailure;

    case BLITZAR_STATUS_INTERNAL_ERROR:

        return Status::InternalError;

    case BLITZAR_STATUS_SINGULARITY:

        return Status::Singularity;

    case BLITZAR_STATUS_UNSUPPORTED:

        return Status::Unsupported;

    default:

        return Status::InternalError;
    }
}

enum class SolverKind : std::int32_t {
    Direct = BLITZAR_SOLVER_DIRECT,
    BarnesHut = BLITZAR_SOLVER_BARNES_HUT,
    Fmm = BLITZAR_SOLVER_FMM,
    Pm = BLITZAR_SOLVER_PM,
    TreePm = BLITZAR_SOLVER_TREEPM
};

enum class BackendKind : std::int32_t { Cpu = BLITZAR_BACKEND_CPU, Hip = BLITZAR_BACKEND_HIP };

enum class IntegratorKind : std::int32_t { LeapfrogKdk = BLITZAR_INTEGRATOR_LEAPFROG_KDK };

class BLITZAR_API Context final {
public:
    Context() noexcept;
    ~Context() noexcept;

    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;

    Context(Context&& other) noexcept;
    Context& operator=(Context&& other) noexcept;

    [[nodiscard]] bool valid() const noexcept;
    [[nodiscard]] Status status() const noexcept;

private:
    struct Impl;

    friend class Simulation;

    std::unique_ptr<Impl> impl_;
    Status status_;
};

class BLITZAR_API Simulation final {
public:
    // The context is only required during construction; it may then be
    // destroyed independently. Calls on one live object are guarded by the C
    // ABI and concurrent reentrant calls return Status::InternalError.
    explicit Simulation(Context& context, std::int64_t particle_count) noexcept;
    ~Simulation() noexcept;

    Simulation(const Simulation&) = delete;
    Simulation& operator=(const Simulation&) = delete;

    Simulation(Simulation&& other) noexcept;
    Simulation& operator=(Simulation&& other) noexcept;

    [[nodiscard]] bool valid() const noexcept;
    [[nodiscard]] Status status() const noexcept;
    [[nodiscard]] std::int64_t particle_count() const noexcept;
    [[nodiscard]] BackendKind backend() const noexcept;

    [[nodiscard]] Status set_solver(SolverKind solver) noexcept;
    [[nodiscard]] Status set_integrator(IntegratorKind integrator) noexcept;
    [[nodiscard]] Status set_gravity(double gravitational_constant, double softening) noexcept;
    [[nodiscard]] Status set_units(
        double length_scale, double mass_scale, double time_scale) noexcept;
    [[nodiscard]] Status set_barnes_hut(double opening_angle, std::int64_t max_particles,
        std::int64_t max_cells, std::int64_t leaf_capacity, std::int64_t max_depth) noexcept;
    [[nodiscard]] Status set_timestep(double timestep) noexcept;
    [[nodiscard]] Status set_seed(std::uint64_t seed) noexcept;
    [[nodiscard]] Status set_particles(std::span<const double> position_x,
        std::span<const double> position_y, std::span<const double> position_z,
        std::span<const double> velocity_x, std::span<const double> velocity_y,
        std::span<const double> velocity_z, std::span<const double> mass) noexcept;
    [[nodiscard]] Status get_state(std::span<double> position_x, std::span<double> position_y,
        std::span<double> position_z, std::span<double> velocity_x, std::span<double> velocity_y,
        std::span<double> velocity_z, std::span<double> mass) noexcept;
    [[nodiscard]] Status step() noexcept;

private:
    struct Impl;

    [[nodiscard]] static bool FitsCount(std::size_t count) noexcept
    {
        return count <= static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max());
    }

    [[nodiscard]] Status Update(blitzar_status status) noexcept;

    std::unique_ptr<Impl> impl_;
    std::atomic<Status> status_;
    std::int64_t particle_count_;
};

} // namespace blitzar

#endif
