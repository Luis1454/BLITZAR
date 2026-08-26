namespace blitzar_hip {

std::uintptr_t Buffers::HostError() const noexcept
{
    return impl_ == nullptr ? 0 : impl_->host_error.Address();
}

std::uintptr_t Buffers::DeviceError() const noexcept
{
    return impl_ == nullptr ? 0 : impl_->device_error.Address();
}

std::uintptr_t Buffers::HostCells() const noexcept
{
    return impl_ == nullptr ? 0 : impl_->host_cells.Address();
}

std::uintptr_t Buffers::DeviceCells() const noexcept
{
    return impl_ == nullptr ? 0 : impl_->device_cells.Address();
}

std::uintptr_t Buffers::HostIndices() const noexcept
{
    return impl_ == nullptr ? 0 : impl_->host_indices.Address();
}

std::uintptr_t Buffers::DeviceIndices() const noexcept
{
    return impl_ == nullptr ? 0 : impl_->device_indices.Address();
}

} // namespace blitzar_hip
