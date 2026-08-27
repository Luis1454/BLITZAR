namespace blitzar_hip {

GpuBuffers::GpuBuffers() noexcept
{
    try {
        impl_ = std::make_unique<Impl>();
    }
    catch (const std::bad_alloc&) {
        impl_.reset();
    }
}

GpuBuffers::~GpuBuffers() noexcept = default;

GpuBuffers::GpuBuffers(GpuBuffers&& other) noexcept = default;

GpuBuffers& GpuBuffers::operator=(GpuBuffers&& other) noexcept = default;

bool GpuBuffers::IsAvailable() const noexcept
{
    return impl_ != nullptr && impl_->available;
}

void GpuBuffers::Disable() noexcept
{
    if (impl_ != nullptr) {
        impl_->available = false;
    }
}

bool GpuBuffers::Ensure(
    std::size_t target_count, std::size_t source_count, std::size_t cell_count) noexcept
{
    return impl_ != nullptr && impl_->available &&
           impl_->Ensure(target_count, source_count, cell_count);
}

} // namespace blitzar_hip
