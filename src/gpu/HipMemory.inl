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

        void* allocation = nullptr;

        if (hipHostMalloc(&allocation, bytes, hipHostMallocDefault) != hipSuccess) {
            return false;
        }

        data_ = reinterpret_cast<std::uintptr_t>(allocation);
        bytes_ = bytes;

        return true;
    }

    void Release() noexcept
    {
        if (data_ != 0) {
            (void)hipHostFree(reinterpret_cast<void*>(data_));

            data_ = 0;
        }

        bytes_ = 0;
    }

    [[nodiscard]] std::uintptr_t Address() const noexcept
    {
        return data_;
    }

private:
    std::uintptr_t data_{};
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

        void* allocation = nullptr;

        if (hipMalloc(&allocation, bytes) != hipSuccess) {
            return false;
        }

        data_ = reinterpret_cast<std::uintptr_t>(allocation);
        bytes_ = bytes;

        return true;
    }

    void Release() noexcept
    {
        if (data_ != 0) {
            (void)hipFree(reinterpret_cast<void*>(data_));

            data_ = 0;
        }

        bytes_ = 0;
    }

    [[nodiscard]] std::uintptr_t Address() const noexcept
    {
        return data_;
    }

private:
    std::uintptr_t data_{};
    std::size_t bytes_{};
};

} // namespace

} // namespace blitzar_gpu
