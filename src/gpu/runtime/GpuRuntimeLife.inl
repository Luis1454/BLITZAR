namespace blitzar_hip {

GpuContext::GpuContext() noexcept
{
    try {
        impl_ = std::make_unique<Impl>();
    }
    catch (const std::bad_alloc&) {
        impl_.reset();

        status_ = BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
}

GpuContext::~GpuContext() noexcept = default;

GpuContext::GpuContext(GpuContext&& other) noexcept = default;

GpuContext& GpuContext::operator=(GpuContext&& other) noexcept = default;

blitzar_status GpuContext::CheckRuntime() const noexcept
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

    return BLITZAR_STATUS_OK;
}

bool GpuContext::IsCompiled() const noexcept
{
    return true;
}

bool GpuContext::IsAvailable() const noexcept
{
    return status_ == BLITZAR_STATUS_OK && impl_ != nullptr && impl_->buffers.IsAvailable();
}

void GpuContext::SetFaultForTesting(Fault fault) noexcept
{
    if (impl_ != nullptr) {
        impl_->fault = fault;
    }
}

blitzar_status GpuContext::ComputeDirect(blitzar_core::ParticleStateView particles,
    blitzar_core::ForceView forces, blitzar_physics::GravityParameters gravity) noexcept
{
    return ComputeDirectRange(particles, forces, gravity, {0, particles.SourceCount(), false});
}

blitzar_status GpuContext::ComputeDirectRange(blitzar_core::ParticleStateView particles,
    blitzar_core::ForceView forces, blitzar_physics::GravityParameters gravity,
    blitzar_solvers::ForceRange range) noexcept
{
    const blitzar_status runtime_status = CheckRuntime();

    if (runtime_status != BLITZAR_STATUS_OK) {
        return runtime_status;
    }
    if (!ValidDirectInput(particles, forces, gravity, range)) {
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

blitzar_status GpuContext::ComputeBarnesHut(const BarnesHutComputeRequest& request) noexcept
{
    const blitzar_status runtime_status = CheckRuntime();

    if (runtime_status != BLITZAR_STATUS_OK) {
        return runtime_status;
    }
    if (!ValidBarnesInput(request)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    if (request.particles.count == 0) {
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

} // namespace blitzar_hip
