namespace blitzar_hip {

struct GpuBuffers::Impl final {
    hipStream_t stream{};
    bool available{};
    std::array<PinnedAllocation, 4> host_particles;
    std::array<DeviceAllocation, 4> device_particles;
    std::array<PinnedAllocation, 4> host_sources;
    std::array<DeviceAllocation, 4> device_sources;
    std::array<PinnedAllocation, 3> host_forces;
    std::array<DeviceAllocation, 3> device_forces;
    PinnedAllocation host_error;
    DeviceAllocation device_error;
    PinnedAllocation host_cells;
    DeviceAllocation device_cells;
    PinnedAllocation host_indices;
    DeviceAllocation device_indices;

    [[nodiscard]] bool Ensure(
        std::size_t target_count, std::size_t source_count, std::size_t cell_count) noexcept;

    Impl() noexcept
    {
        int device_count = 0;

        if (hipGetDeviceCount(&device_count) != hipSuccess || device_count <= 0 ||
            hipSetDevice(0) != hipSuccess ||
            hipStreamCreateWithFlags(&stream, hipStreamNonBlocking) != hipSuccess) {
            return;
        }

        available = true;
    }

    ~Impl()
    {
        if (stream != nullptr) {
            (void)hipStreamSynchronize(stream);
            (void)hipStreamDestroy(stream);

            stream = nullptr;
        }
    }
};

} // namespace blitzar_hip
