namespace blitzar_hip {

Buffers::Buffers() noexcept
{
    try {
        impl_ = std::make_unique<Impl>();
    }
    catch (const std::bad_alloc&) {
        impl_.reset();
    }
}

Buffers::~Buffers() noexcept = default;

Buffers::Buffers(Buffers&& other) noexcept = default;

Buffers& Buffers::operator=(Buffers&& other) noexcept = default;

bool Buffers::IsAvailable() const noexcept
{
    return impl_ != nullptr && impl_->available;
}

void Buffers::Disable() noexcept
{
    if (impl_ != nullptr) {
        impl_->available = false;
    }
}

bool Buffers::Ensure(
    std::size_t target_count, std::size_t source_count, std::size_t cell_count) noexcept
{
    return impl_ != nullptr && impl_->available &&
           impl_->Ensure(target_count, source_count, cell_count);
}

} // namespace blitzar_hip
