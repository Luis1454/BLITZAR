/*
 * @file engine/physics/cuda/include/DeviceBuffers.inl
 * @project BLITZAR
 * @brief Inline ownership types for device and mapped-host buffers.
 */

#include <memory>

namespace blitzar_cuda_memory {

template <typename T>
struct DeviceDeleter final {
    void operator()(T* pointer) const noexcept
    {
        deallocate(static_cast<void*>(pointer));
    }
};

template <typename T>
struct MappedHostDeleter final {
    void operator()(T* pointer) const noexcept
    {
        releaseMappedHost(static_cast<void*>(pointer));
    }
};

template <typename T>
class DeviceBuffer final {
public:
    DeviceBuffer() noexcept = default;

    explicit DeviceBuffer(T* pointer) noexcept : _owner(pointer)
    {
    }

    DeviceBuffer(const DeviceBuffer&) = delete;
    DeviceBuffer& operator=(const DeviceBuffer&) = delete;
    DeviceBuffer(DeviceBuffer&&) noexcept = default;
    DeviceBuffer& operator=(DeviceBuffer&&) noexcept = default;
    ~DeviceBuffer() = default;

    DeviceBuffer& operator=(T* pointer) noexcept
    {
        reset(pointer);
        return *this;
    }

    DeviceBuffer& operator=(void* pointer) noexcept
    {
        reset(static_cast<T*>(pointer));
        return *this;
    }

    DeviceBuffer& operator=(std::nullptr_t) noexcept
    {
        reset();
        return *this;
    }

    T* get() noexcept
    {
        return _owner.get();
    }

    T* get() const noexcept
    {
        return _owner.get();
    }

    operator T*() noexcept
    {
        return get();
    }

    operator T*() const noexcept
    {
        return get();
    }

    void reset(T* pointer = nullptr) noexcept
    {
        _owner.reset(pointer);
    }

    void swap(DeviceBuffer& other) noexcept
    {
        _owner.swap(other._owner);
    }

    T* release() noexcept
    {
        return _owner.release();
    }

private:
    std::unique_ptr<T, DeviceDeleter<T>> _owner;
};

template <typename T>
class MappedHostBuffer final {
public:
    MappedHostBuffer() noexcept = default;

    MappedHostBuffer(const MappedHostBuffer&) = delete;
    MappedHostBuffer& operator=(const MappedHostBuffer&) = delete;
    MappedHostBuffer(MappedHostBuffer&&) noexcept = default;
    MappedHostBuffer& operator=(MappedHostBuffer&&) noexcept = default;
    ~MappedHostBuffer() = default;

    MappedHostBuffer& operator=(T* pointer) noexcept
    {
        reset(pointer);
        return *this;
    }

    MappedHostBuffer& operator=(std::nullptr_t) noexcept
    {
        reset();
        return *this;
    }

    T* get() noexcept
    {
        return _owner.get();
    }

    const T* get() const noexcept
    {
        return _owner.get();
    }

    operator T*() noexcept
    {
        return get();
    }

    operator const T*() const noexcept
    {
        return get();
    }

    explicit operator bool() const noexcept
    {
        return static_cast<bool>(_owner);
    }

    void reset(T* pointer = nullptr) noexcept
    {
        _owner.reset(pointer);
    }

private:
    std::unique_ptr<T, MappedHostDeleter<T>> _owner;
};

} // namespace blitzar_cuda_memory
