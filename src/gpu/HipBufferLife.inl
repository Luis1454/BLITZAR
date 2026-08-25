namespace blitzar_gpu {

HipBuffers::HipBuffers() noexcept
{
    try {
        impl_ = std::make_unique<Impl>();
    }
    catch (const std::bad_alloc&) {
        impl_.reset();
    }
}

HipBuffers::~HipBuffers() noexcept = default;

HipBuffers::HipBuffers(HipBuffers&& other) noexcept = default;

HipBuffers& HipBuffers::operator=(HipBuffers&& other) noexcept = default;

bool HipBuffers::IsAvailable() const noexcept
{
    return impl_ != nullptr && impl_->available;
}

void HipBuffers::Disable() noexcept
{
    if (impl_ != nullptr) {
        impl_->available = false;
    }
}

bool HipBuffers::Ensure(
    std::size_t target_count, std::size_t source_count, std::size_t cell_count) noexcept
{
    return impl_ != nullptr && impl_->available &&
           impl_->Ensure(target_count, source_count, cell_count);
}

} // namespace blitzar_gpu
