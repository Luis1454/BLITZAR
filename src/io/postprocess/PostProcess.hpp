#ifndef BLITZAR_IO_POSTPROCESS_POST_PROCESS_HPP
#define BLITZAR_IO_POSTPROCESS_POST_PROCESS_HPP

#include <blitzar/blitzar.h>
#include <cstdint>
#include <filesystem>

namespace blitzar_io {

struct PostProcessReport final {
    std::uint64_t requested_steps{};
    std::uint64_t completed_steps{};
    std::uint64_t particle_count{};
    blitzar_solver_kind solver{BLITZAR_SOLVER_DIRECT};
    std::uint64_t snapshot_count{};
    std::uint64_t diagnostics_count{};
    std::filesystem::path output_path;
};

class PostProcess final {
public:
    explicit PostProcess(std::filesystem::path run_directory);

    [[nodiscard]] blitzar_status Execute(PostProcessReport& report) noexcept;

private:
    std::filesystem::path run_directory_;
};

} // namespace blitzar_io

#endif
