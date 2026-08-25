namespace blitzar_gpu {

std::uintptr_t HipBuffers::Stream() const noexcept
{
    return impl_ == nullptr ? 0 : reinterpret_cast<std::uintptr_t>(impl_->stream);
}

std::uintptr_t HipBuffers::HostParticle(std::size_t index) const noexcept
{
    return impl_ == nullptr || index >= impl_->host_particles.size()
               ? 0
               : impl_->host_particles[index].Address();
}

std::uintptr_t HipBuffers::DeviceParticle(std::size_t index) const noexcept
{
    return impl_ == nullptr || index >= impl_->device_particles.size()
               ? 0
               : impl_->device_particles[index].Address();
}

std::uintptr_t HipBuffers::HostSource(std::size_t index) const noexcept
{
    return impl_ == nullptr || index >= impl_->host_sources.size()
               ? 0
               : impl_->host_sources[index].Address();
}

std::uintptr_t HipBuffers::DeviceSource(std::size_t index) const noexcept
{
    return impl_ == nullptr || index >= impl_->device_sources.size()
               ? 0
               : impl_->device_sources[index].Address();
}

std::uintptr_t HipBuffers::HostForce(std::size_t index) const noexcept
{
    return impl_ == nullptr || index >= impl_->host_forces.size()
               ? 0
               : impl_->host_forces[index].Address();
}

std::uintptr_t HipBuffers::DeviceForce(std::size_t index) const noexcept
{
    return impl_ == nullptr || index >= impl_->device_forces.size()
               ? 0
               : impl_->device_forces[index].Address();
}

} // namespace blitzar_gpu
