namespace blitzar_gpu {

namespace {

class PinnedAllocation final {
public:
    PinnedAllocation() = default;
    PinnedAllocation(const PinnedAllocation&) = delete;
    PinnedAllocation& operator=(const PinnedAllocation&) = delete;
    ~PinnedAllocation()
    {
        Release();
    }

    [[nodiscard]] bool Resize(std::size_t bytes) noexcept
    {
        if (bytes <= bytes_) {
            return true;
        }

        Release();

        if (bytes == 0) {
            return true;
        }
        if (hipHostMalloc(&data_, bytes, hipHostMallocDefault) != hipSuccess) {
            return false;
        }

        bytes_ = bytes;

        return true;
    }

    void Release() noexcept
    {
        if (data_ != nullptr) {
            (void)hipHostFree(data_);

            data_ = nullptr;
        }

        bytes_ = 0;
    }

    [[nodiscard]] std::uintptr_t Address() const noexcept
    {
        return reinterpret_cast<std::uintptr_t>(data_);
    }

private:
    void* data_{};
    std::size_t bytes_{};
};

class DeviceAllocation final {
public:
    DeviceAllocation() = default;
    DeviceAllocation(const DeviceAllocation&) = delete;
    DeviceAllocation& operator=(const DeviceAllocation&) = delete;
    ~DeviceAllocation()
    {
        Release();
    }

    [[nodiscard]] bool Resize(std::size_t bytes) noexcept
    {
        if (bytes <= bytes_) {
            return true;
        }

        Release();

        if (bytes == 0) {
            return true;
        }
        if (hipMalloc(&data_, bytes) != hipSuccess) {
            return false;
        }

        bytes_ = bytes;

        return true;
    }

    void Release() noexcept
    {
        if (data_ != nullptr) {
            (void)hipFree(data_);

            data_ = nullptr;
        }

        bytes_ = 0;
    }

    [[nodiscard]] std::uintptr_t Address() const noexcept
    {
        return reinterpret_cast<std::uintptr_t>(data_);
    }

private:
    void* data_{};
    std::size_t bytes_{};
};

} // namespace

} // namespace blitzar_gpu
