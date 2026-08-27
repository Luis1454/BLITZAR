namespace blitzar_hip {

std::uintptr_t GpuBuffers::Stream() const noexcept
{
    return impl_ == nullptr ? 0 : reinterpret_cast<std::uintptr_t>(impl_->stream);
}

std::uintptr_t GpuBuffers::HostParticle(std::size_t index) const noexcept
{
    return impl_ == nullptr || index >= impl_->host_particles.size()
               ? 0
               : impl_->host_particles[index].Address();
}

std::uintptr_t GpuBuffers::DeviceParticle(std::size_t index) const noexcept
{
    return impl_ == nullptr || index >= impl_->device_particles.size()
               ? 0
               : impl_->device_particles[index].Address();
}

std::uintptr_t GpuBuffers::HostSource(std::size_t index) const noexcept
{
    return impl_ == nullptr || index >= impl_->host_sources.size()
               ? 0
               : impl_->host_sources[index].Address();
}

std::uintptr_t GpuBuffers::DeviceSource(std::size_t index) const noexcept
{
    return impl_ == nullptr || index >= impl_->device_sources.size()
               ? 0
               : impl_->device_sources[index].Address();
}

std::uintptr_t GpuBuffers::HostForce(std::size_t index) const noexcept
{
    return impl_ == nullptr || index >= impl_->host_forces.size()
               ? 0
               : impl_->host_forces[index].Address();
}

std::uintptr_t GpuBuffers::DeviceForce(std::size_t index) const noexcept
{
    return impl_ == nullptr || index >= impl_->device_forces.size()
               ? 0
               : impl_->device_forces[index].Address();
}

} // namespace blitzar_hip
