/*
 * @file engine/physics/core/cuda/core/CudHandles.inl
 * @project BLITZAR
 * @brief Inline ownership types for opaque CUDA runtime handles.
 */

#include <memory>

namespace blitzar_cuda_memory {

template <void (*Release)(void*) noexcept>
class OpaqueHandle final {
public:
    OpaqueHandle() noexcept = default;

    OpaqueHandle(const OpaqueHandle&) = delete;
    OpaqueHandle& operator=(const OpaqueHandle&) = delete;
    OpaqueHandle(OpaqueHandle&&) noexcept = default;
    OpaqueHandle& operator=(OpaqueHandle&&) noexcept = default;
    ~OpaqueHandle() = default;

    OpaqueHandle& operator=(void* handle) noexcept
    {
        reset(handle);
        return *this;
    }

    OpaqueHandle& operator=(std::nullptr_t) noexcept
    {
        reset();
        return *this;
    }

    void* get() noexcept
    {
        return _owner.get();
    }

    void* get() const noexcept
    {
        return _owner.get();
    }

    explicit operator bool() const noexcept
    {
        return get() != nullptr;
    }

    void reset(void* handle = nullptr) noexcept
    {
        _owner.reset(handle);
    }

private:
    struct Deleter final {
        void operator()(void* handle) const noexcept
        {
            Release(handle);
        }
    };

    std::unique_ptr<void, Deleter> _owner;
};

class FftPlanHandle final {
public:
    FftPlanHandle() noexcept = default;

    FftPlanHandle(const FftPlanHandle&) = delete;
    FftPlanHandle& operator=(const FftPlanHandle&) = delete;
    FftPlanHandle(FftPlanHandle&&) noexcept = default;
    FftPlanHandle& operator=(FftPlanHandle&&) noexcept = default;
    ~FftPlanHandle() = default;

    FftPlanHandle& operator=(int handle) noexcept
    {
        reset(handle);
        return *this;
    }

    operator int() const noexcept
    {
        return _handle;
    }

    int get() const noexcept
    {
        return _handle;
    }

    void reset(int handle = 0) noexcept
    {
        if (_handle != 0) {
            releaseFftPlan(_handle);
        }
        _handle = handle;
    }

private:
    int _handle = 0;
};

} // namespace blitzar_cuda_memory
