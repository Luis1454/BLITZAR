namespace blitzar_gpu {

std::uintptr_t HipBuffers::HostError() const noexcept
{
    return impl_ == nullptr ? 0 : impl_->host_error.Address();
}

std::uintptr_t HipBuffers::DeviceError() const noexcept
{
    return impl_ == nullptr ? 0 : impl_->device_error.Address();
}

std::uintptr_t HipBuffers::HostCells() const noexcept
{
    return impl_ == nullptr ? 0 : impl_->host_cells.Address();
}

std::uintptr_t HipBuffers::DeviceCells() const noexcept
{
    return impl_ == nullptr ? 0 : impl_->device_cells.Address();
}

std::uintptr_t HipBuffers::HostIndices() const noexcept
{
    return impl_ == nullptr ? 0 : impl_->host_indices.Address();
}

std::uintptr_t HipBuffers::DeviceIndices() const noexcept
{
    return impl_ == nullptr ? 0 : impl_->device_indices.Address();
}

} // namespace blitzar_gpu
