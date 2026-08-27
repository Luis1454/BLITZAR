namespace blitzar_hip {

std::uintptr_t GpuBuffers::HostError() const noexcept
{
    return impl_ == nullptr ? 0 : impl_->host_error.Address();
}

std::uintptr_t GpuBuffers::DeviceError() const noexcept
{
    return impl_ == nullptr ? 0 : impl_->device_error.Address();
}

std::uintptr_t GpuBuffers::HostCells() const noexcept
{
    return impl_ == nullptr ? 0 : impl_->host_cells.Address();
}

std::uintptr_t GpuBuffers::DeviceCells() const noexcept
{
    return impl_ == nullptr ? 0 : impl_->device_cells.Address();
}

std::uintptr_t GpuBuffers::HostIndices() const noexcept
{
    return impl_ == nullptr ? 0 : impl_->host_indices.Address();
}

std::uintptr_t GpuBuffers::DeviceIndices() const noexcept
{
    return impl_ == nullptr ? 0 : impl_->device_indices.Address();
}

} // namespace blitzar_hip
