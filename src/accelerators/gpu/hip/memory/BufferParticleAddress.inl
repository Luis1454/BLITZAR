namespace blitzar_hip {

std::uintptr_t Buffers::Stream() const noexcept
{
    return impl_ == nullptr ? 0 : reinterpret_cast<std::uintptr_t>(impl_->stream);
}

std::uintptr_t Buffers::HostParticle(std::size_t index) const noexcept
{
    return impl_ == nullptr || index >= impl_->host_particles.size()
               ? 0
               : impl_->host_particles[index].Address();
}

std::uintptr_t Buffers::DeviceParticle(std::size_t index) const noexcept
{
    return impl_ == nullptr || index >= impl_->device_particles.size()
               ? 0
               : impl_->device_particles[index].Address();
}

std::uintptr_t Buffers::HostSource(std::size_t index) const noexcept
{
    return impl_ == nullptr || index >= impl_->host_sources.size()
               ? 0
               : impl_->host_sources[index].Address();
}

std::uintptr_t Buffers::DeviceSource(std::size_t index) const noexcept
{
    return impl_ == nullptr || index >= impl_->device_sources.size()
               ? 0
               : impl_->device_sources[index].Address();
}

std::uintptr_t Buffers::HostForce(std::size_t index) const noexcept
{
    return impl_ == nullptr || index >= impl_->host_forces.size()
               ? 0
               : impl_->host_forces[index].Address();
}

std::uintptr_t Buffers::DeviceForce(std::size_t index) const noexcept
{
    return impl_ == nullptr || index >= impl_->device_forces.size()
               ? 0
               : impl_->device_forces[index].Address();
}

} // namespace blitzar_hip
