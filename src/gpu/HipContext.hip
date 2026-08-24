#include "gpu/HipBuffers.hpp"
#include "gpu/HipContext.hpp"
#include "solvers/gpu/HipKernel.hpp"

#if defined(BLITZAR_HIP_NATIVE_CUDA)
#include "gpu/HipCompat.hpp"
#else
#include <hip/hip_runtime.h>
#endif

#include "gpu/CudRuntime.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <memory>
#include <new>
#include <stdexcept>
#include <utility>

namespace blitzar_gpu {

namespace {

[[nodiscard]] blitzar_status HipStatus(hipError_t error) noexcept
{
    return error == hipErrorMemoryAllocation ? BLITZAR_STATUS_ALLOCATION_FAILURE
                                             : BLITZAR_STATUS_INTERNAL_ERROR;
}

[[nodiscard]] blitzar_status FaultStatus(HipFault fault) noexcept
{
    switch (fault) {
    case HipFault::None:

        return BLITZAR_STATUS_OK;

    case HipFault::AllocationFailure:

        return BLITZAR_STATUS_ALLOCATION_FAILURE;

    case HipFault::LaunchFailure:
    case HipFault::SynchronizationFailure:

        return BLITZAR_STATUS_INTERNAL_ERROR;

    case HipFault::NonFiniteResult:

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

} // namespace

struct HipContext::Impl final {
    HipBuffers buffers;
    std::unique_ptr<blitzar_trees::Octree> tree;
    blitzar_barnes_hut::BarnesHutSettings tree_settings{};
    bool tree_ready{};
    HipFault fault{HipFault::None};

    [[nodiscard]] blitzar_status EnsureBuffers(std::size_t target_count,
        std::size_t source_count, std::size_t cell_count) noexcept
    {
        return buffers.Ensure(target_count, source_count, cell_count)
                   ? BLITZAR_STATUS_OK
                   : BLITZAR_STATUS_ALLOCATION_FAILURE;
    }

    [[nodiscard]] blitzar_status UploadFullState(
        blitzar_core::ParticleStateView particles) noexcept
    {
        const std::array<std::span<const double>, 4> source{
            particles.x, particles.y, particles.z, particles.mass};

        for (std::size_t index = 0; index < source.size(); ++index) {
            std::copy(source[index].begin(), source[index].end(),
                reinterpret_cast<double*>(buffers.HostParticle(index)));

            const hipError_t error = BlitzarHipMemcpyAsync(
                {reinterpret_cast<void*>(buffers.DeviceParticle(index)),
                    reinterpret_cast<const void*>(buffers.HostParticle(index)),
                    source[index].size_bytes(), hipMemcpyHostToDevice,
                    reinterpret_cast<hipStream_t>(buffers.Stream())});

            if (error != hipSuccess) {
                buffers.Disable();

                return HipStatus(error);
            }
        }

        return BLITZAR_STATUS_OK;
    }

    [[nodiscard]] blitzar_status UploadTargetState(
        blitzar_core::ParticleStateView particles) noexcept
    {
        const std::array<std::span<const double>, 4> source{
            particles.x.first(particles.count), particles.y.first(particles.count),
            particles.z.first(particles.count), particles.mass.first(particles.count)};

        for (std::size_t index = 0; index < source.size(); ++index) {
            std::copy(source[index].begin(), source[index].end(),
                reinterpret_cast<double*>(buffers.HostParticle(index)));

            const hipError_t error = BlitzarHipMemcpyAsync(
                {reinterpret_cast<void*>(buffers.DeviceParticle(index)),
                    reinterpret_cast<const void*>(buffers.HostParticle(index)),
                    source[index].size_bytes(), hipMemcpyHostToDevice,
                    reinterpret_cast<hipStream_t>(buffers.Stream())});

            if (error != hipSuccess) {
                buffers.Disable();

                return HipStatus(error);
            }
        }

        return BLITZAR_STATUS_OK;
    }

    [[nodiscard]] blitzar_status UploadSourceState(blitzar_core::ParticleStateView particles,
        blitzar_core::ForceRange range) noexcept
    {
        const std::size_t source_count = range.source_end - range.source_begin;
        const std::array<std::span<const double>, 4> source{
            particles.x.subspan(range.source_begin, source_count),
            particles.y.subspan(range.source_begin, source_count),
            particles.z.subspan(range.source_begin, source_count),
            particles.mass.subspan(range.source_begin, source_count)};

        for (std::size_t index = 0; index < source.size(); ++index) {
            std::copy(source[index].begin(), source[index].end(),
                reinterpret_cast<double*>(buffers.HostSource(index)));

            const hipError_t error = BlitzarHipMemcpyAsync(
                {reinterpret_cast<void*>(buffers.DeviceSource(index)),
                    reinterpret_cast<const void*>(buffers.HostSource(index)),
                    source[index].size_bytes(), hipMemcpyHostToDevice,
                    reinterpret_cast<hipStream_t>(buffers.Stream())});

            if (error != hipSuccess) {
                buffers.Disable();

                return HipStatus(error);
            }
        }

        return BLITZAR_STATUS_OK;
    }

    [[nodiscard]] blitzar_status UploadForces(blitzar_core::ForceView forces) noexcept
    {
        const std::array<std::span<const double>, 3> source{forces.x, forces.y, forces.z};

        for (std::size_t index = 0; index < source.size(); ++index) {
            std::copy(source[index].begin(), source[index].end(),
                reinterpret_cast<double*>(buffers.HostForce(index)));

            const hipError_t error = BlitzarHipMemcpyAsync(
                {reinterpret_cast<void*>(buffers.DeviceForce(index)),
                    reinterpret_cast<const void*>(buffers.HostForce(index)),
                    source[index].size_bytes(), hipMemcpyHostToDevice,
                    reinterpret_cast<hipStream_t>(buffers.Stream())});

            if (error != hipSuccess) {
                buffers.Disable();

                return HipStatus(error);
            }
        }

        return BLITZAR_STATUS_OK;
    }

    [[nodiscard]] blitzar_status QueueForceDownloads(std::size_t particle_count) noexcept
    {
        const std::size_t bytes = particle_count * sizeof(double);

        for (std::size_t index = 0; index < 3; ++index) {
            const hipError_t error = BlitzarHipMemcpyAsync(
                {reinterpret_cast<void*>(buffers.HostForce(index)),
                    reinterpret_cast<const void*>(buffers.DeviceForce(index)), bytes,
                    hipMemcpyDeviceToHost, reinterpret_cast<hipStream_t>(buffers.Stream())});

            if (error != hipSuccess) {
                buffers.Disable();

                return HipStatus(error);
            }
        }

        return BLITZAR_STATUS_OK;
    }

    [[nodiscard]] blitzar_status Finish(
        std::size_t particle_count, blitzar_core::ForceView forces) noexcept
    {
        const hipError_t synchronization =
            hipStreamSynchronize(reinterpret_cast<hipStream_t>(buffers.Stream()));

        if (synchronization != hipSuccess) {
            buffers.Disable();

            return HipStatus(synchronization);
        }

        const int device_status = *reinterpret_cast<const int*>(buffers.HostError());

        if (device_status != 0) {
            return static_cast<blitzar_status>(device_status);
        }

        const std::array<std::span<double>, 3> destination{forces.x, forces.y, forces.z};

        for (std::size_t index = 0; index < destination.size(); ++index) {
            std::copy_n(reinterpret_cast<const double*>(buffers.HostForce(index)), particle_count,
                destination[index].begin());
        }

        return BLITZAR_STATUS_OK;
    }

    [[nodiscard]] blitzar_status ComputeDirect(blitzar_core::ParticleStateView particles,
        blitzar_core::ForceView forces, blitzar_physics::GravityParameters gravity,
        blitzar_core::ForceRange range) noexcept
    {
        const std::size_t source_count = range.source_end - range.source_begin;
        const bool source_alias_target = range.source_begin == 0 &&
                                         source_count == particles.count &&
                                         particles.SourceCount() == particles.count;

        const blitzar_status buffer_status =
            EnsureBuffers(particles.count, source_alias_target ? 0 : source_count, 0);

        if (buffer_status != BLITZAR_STATUS_OK) {
            return buffer_status;
        }

        blitzar_status status = UploadTargetState(particles);

        if (status == BLITZAR_STATUS_OK && !source_alias_target && source_count != 0) {
            status = UploadSourceState(particles, range);
        }
        if (status == BLITZAR_STATUS_OK && range.accumulate) {
            status = UploadForces(forces);
        }

        if (status != BLITZAR_STATUS_OK) {
            return status;
        }
        if (hipMemsetAsync(reinterpret_cast<void*>(buffers.DeviceError()), 0, sizeof(int),
                reinterpret_cast<hipStream_t>(buffers.Stream())) != hipSuccess) {
            buffers.Disable();

            return BLITZAR_STATUS_INTERNAL_ERROR;
        }

        const blitzar_gpu_detail::DeviceParticleAddresses addresses{buffers.DeviceParticle(0),
            buffers.DeviceParticle(1), buffers.DeviceParticle(2), buffers.DeviceParticle(3),
            buffers.DeviceForce(0), buffers.DeviceForce(1), buffers.DeviceForce(2),
            source_alias_target ? buffers.DeviceParticle(0) : buffers.DeviceSource(0),
            source_alias_target ? buffers.DeviceParticle(1) : buffers.DeviceSource(1),
            source_alias_target ? buffers.DeviceParticle(2) : buffers.DeviceSource(2),
            source_alias_target ? buffers.DeviceParticle(3) : buffers.DeviceSource(3)};

        const blitzar_gpu_detail::DirectLaunchRequest launch_request{
            addresses, particles.count, {0, source_count, range.accumulate},

            {gravity.EffectiveConstant(), gravity.EffectiveSoftening()},

            {buffers.DeviceError(), buffers.Stream()}, range.source_begin};

        status = blitzar_gpu_detail::LaunchDirect(launch_request);

        if (status != BLITZAR_STATUS_OK) {
            buffers.Disable();

            return status;
        }
        if (BlitzarHipMemcpyAsync(
                {reinterpret_cast<void*>(buffers.HostError()),
                    reinterpret_cast<const void*>(buffers.DeviceError()), sizeof(int),
                    hipMemcpyDeviceToHost, reinterpret_cast<hipStream_t>(buffers.Stream())}) !=

            hipSuccess) {
            buffers.Disable();

            return BLITZAR_STATUS_INTERNAL_ERROR;
        }

        status = QueueForceDownloads(particles.count);

        return status == BLITZAR_STATUS_OK ? Finish(particles.count, forces) : status;
    }

    [[nodiscard]] blitzar_status EnsureTree(blitzar_barnes_hut::BarnesHutSettings settings) noexcept
    {
        if (tree == nullptr || !tree_ready || !SameSettings(tree_settings, settings)) {
            try {
                tree = std::make_unique<blitzar_trees::Octree>(settings.max_particles,
                    settings.max_cells, settings.leaf_capacity, settings.max_depth);
            }
            catch (const std::bad_alloc&) {
                return BLITZAR_STATUS_ALLOCATION_FAILURE;
            }
            catch (const std::length_error&) {
                return BLITZAR_STATUS_INVALID_ARGUMENT;
            }

            tree_settings = settings;
            tree_ready = false;
        }

        return BLITZAR_STATUS_OK;
    }

    [[nodiscard]] blitzar_status ComputeBarnesHut(
        const BarnesHutComputeRequest& request) noexcept
    {
        const blitzar_core::ParticleStateView particles = request.particles;
        const blitzar_core::ForceView forces = request.forces;
        const blitzar_core::ExecutionSettings& execution = request.execution;
        const blitzar_physics::GravityParameters gravity = request.gravity;
        const blitzar_barnes_hut::BarnesHutSettings settings = request.settings;

        (void)execution;

        blitzar_status status = EnsureTree(settings);

        if (status != BLITZAR_STATUS_OK) {
            return status;
        }
        if (!tree->Refit(particles)) {
            status = tree->Build(particles);

            if (status != BLITZAR_STATUS_OK) {
                return status;
            }
        }

        tree_ready = true;

        const std::span<const blitzar_trees::Octree::Cell> cells = tree->Cells();
        const std::span<const std::size_t> indices = tree->Indices();

        status = EnsureBuffers(particles.SourceCount(), 0, cells.size());

        if (status != BLITZAR_STATUS_OK) {
            return status;
        }

        auto* host_cell_data = reinterpret_cast<blitzar_gpu_detail::GpuCell*>(buffers.HostCells());

        for (std::size_t index = 0; index < cells.size(); ++index) {
            const blitzar_trees::Octree::Cell& source = cells[index];

            blitzar_gpu_detail::GpuCell& destination = host_cell_data[index];
            destination.center_x = source.center.x;
            destination.center_y = source.center.y;
            destination.center_z = source.center.z;
            destination.center_of_mass_x = source.center_of_mass.x;
            destination.center_of_mass_y = source.center_of_mass.y;
            destination.center_of_mass_z = source.center_of_mass.z;
            destination.half_extent = source.half_extent;
            destination.mass = source.mass;
            destination.begin = static_cast<std::uint64_t>(source.begin);
            destination.count = static_cast<std::uint64_t>(source.count);

            for (std::size_t child = 0; child < source.children.size(); ++child) {
                destination.children[child] =
                    source.children[child] == blitzar_trees::Octree::Cell::InvalidIndex
                        ? std::numeric_limits<std::uint64_t>::max()
                        : static_cast<std::uint64_t>(source.children[child]);
            }
        }

        auto* host_index_data = reinterpret_cast<std::uint64_t*>(buffers.HostIndices());

        for (std::size_t index = 0; index < indices.size(); ++index) {
            host_index_data[index] = static_cast<std::uint64_t>(indices[index]);
        }

        status = UploadFullState(particles);

        if (status != BLITZAR_STATUS_OK) {
            return status;
        }
        if (BlitzarHipMemcpyAsync(
                {reinterpret_cast<void*>(buffers.DeviceCells()),
                    reinterpret_cast<const void*>(buffers.HostCells()),
                    cells.size() * sizeof(blitzar_gpu_detail::GpuCell), hipMemcpyHostToDevice,
                    reinterpret_cast<hipStream_t>(buffers.Stream())}) != hipSuccess ||

            BlitzarHipMemcpyAsync(
                {reinterpret_cast<void*>(buffers.DeviceIndices()),
                    reinterpret_cast<const void*>(buffers.HostIndices()),
                    indices.size() * sizeof(std::uint64_t), hipMemcpyHostToDevice,
                    reinterpret_cast<hipStream_t>(buffers.Stream())}) != hipSuccess ||
            hipMemsetAsync(reinterpret_cast<void*>(buffers.DeviceError()), 0, sizeof(int),
                reinterpret_cast<hipStream_t>(buffers.Stream())) != hipSuccess) {
            buffers.Disable();

            return BLITZAR_STATUS_INTERNAL_ERROR;
        }

        const blitzar_gpu_detail::DeviceParticleAddresses addresses{buffers.DeviceParticle(0),
            buffers.DeviceParticle(1), buffers.DeviceParticle(2), buffers.DeviceParticle(3),
            buffers.DeviceForce(0), buffers.DeviceForce(1), buffers.DeviceForce(2)};

        const blitzar_gpu_detail::BarnesHutLaunchRequest launch_request{
            addresses,
            particles.count,
            particles.SourceCount(),
            settings.opening_angle,

            {buffers.DeviceCells(), cells.size(), buffers.DeviceIndices()},

            {gravity.EffectiveConstant(), gravity.EffectiveSoftening()},

            settings.max_depth,

            {buffers.DeviceError(), buffers.Stream()}};

        status = blitzar_gpu_detail::LaunchBarnesHut(launch_request);

        if (status != BLITZAR_STATUS_OK) {
            buffers.Disable();

            return status;
        }
        if (BlitzarHipMemcpyAsync(
                {reinterpret_cast<void*>(buffers.HostError()),
                    reinterpret_cast<const void*>(buffers.DeviceError()), sizeof(int),
                    hipMemcpyDeviceToHost, reinterpret_cast<hipStream_t>(buffers.Stream())}) !=

            hipSuccess) {
            buffers.Disable();

            return BLITZAR_STATUS_INTERNAL_ERROR;
        }

        status = QueueForceDownloads(particles.count);

        return status == BLITZAR_STATUS_OK ? Finish(particles.count, forces) : status;
    }
};

HipContext::HipContext() noexcept
{
    try {
        impl_ = std::make_unique<Impl>();
    }
    catch (const std::bad_alloc&) {
        impl_.reset();

        status_ = BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
}

HipContext::~HipContext() noexcept = default;

HipContext::HipContext(HipContext&& other) noexcept = default;

HipContext& HipContext::operator=(HipContext&& other) noexcept = default;

bool HipContext::IsCompiled() const noexcept
{
    return true;
}

bool HipContext::IsAvailable() const noexcept
{
    return status_ == BLITZAR_STATUS_OK && impl_ != nullptr && impl_->buffers.IsAvailable();
}

void HipContext::SetFaultForTesting(HipFault fault) noexcept
{
    if (impl_ != nullptr) {
        impl_->fault = fault;
    }
}

blitzar_status HipContext::ComputeDirect(blitzar_core::ParticleStateView particles,
    blitzar_core::ForceView forces, blitzar_physics::GravityParameters gravity) noexcept
{
    return ComputeDirectRange(particles, forces, gravity, {0, particles.SourceCount(), false});
}

blitzar_status HipContext::ComputeDirectRange(blitzar_core::ParticleStateView particles,
    blitzar_core::ForceView forces, blitzar_physics::GravityParameters gravity,
    blitzar_core::ForceRange range) noexcept
{
    if (status_ != BLITZAR_STATUS_OK) {
        return status_;
    }
    if (impl_ != nullptr) {
        const blitzar_status fault_status = FaultStatus(impl_->fault);

        if (fault_status != BLITZAR_STATUS_OK) {
            return fault_status;
        }
    }
    if (impl_ == nullptr || !impl_->buffers.IsAvailable()) {
        return BLITZAR_STATUS_UNSUPPORTED;
    }
    if (!blitzar_core::IsValid(particles) || !blitzar_core::IsValid(forces) ||
        particles.count != forces.count || !gravity.IsValid() ||
        !range.IsValid(particles.SourceCount())) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    if (particles.count == 0) {
        return BLITZAR_STATUS_OK;
    }

    try {
        return impl_->ComputeDirect(particles, forces, gravity, range);
    }
    catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
    catch (const std::length_error&) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
}

blitzar_status HipContext::ComputeBarnesHut(const BarnesHutComputeRequest& request) noexcept
{
    const blitzar_core::ParticleStateView particles = request.particles;
    const blitzar_core::ForceView forces = request.forces;
    const blitzar_core::ExecutionSettings& execution = request.execution;
    const blitzar_physics::GravityParameters gravity = request.gravity;
    const blitzar_barnes_hut::BarnesHutSettings settings = request.settings;

    if (status_ != BLITZAR_STATUS_OK) {
        return status_;
    }
    if (impl_ != nullptr) {
        const blitzar_status fault_status = FaultStatus(impl_->fault);

        if (fault_status != BLITZAR_STATUS_OK) {
            return fault_status;
        }
    }
    if (impl_ == nullptr || !impl_->buffers.IsAvailable()) {
        return BLITZAR_STATUS_UNSUPPORTED;
    }
    if (!blitzar_core::IsValid(particles) || !blitzar_core::IsValid(forces) ||
        particles.count != forces.count || !execution.IsValid() || !gravity.IsValid() ||
        !settings.IsValid() || particles.SourceCount() > settings.max_particles) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    if (particles.count == 0) {
        return BLITZAR_STATUS_OK;
    }

    try {
        return impl_->ComputeBarnesHut(request);
    }
    catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
    catch (const std::length_error&) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
}

} // namespace blitzar_gpu
