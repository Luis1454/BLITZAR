#ifndef BLITZAR_IO_POSTPROCESS_POST_PROCESS_OUTPUT_HPP
#define BLITZAR_IO_POSTPROCESS_POST_PROCESS_OUTPUT_HPP

#include <blitzar/blitzar.h>
#include <filesystem>

namespace blitzar_io {

class PostProcessOutput final {
public:
    explicit PostProcessOutput(std::filesystem::path directory);
    ~PostProcessOutput();

    PostProcessOutput(const PostProcessOutput&) = delete;
    PostProcessOutput& operator=(const PostProcessOutput&) = delete;

    [[nodiscard]] blitzar_status Prepare() noexcept;
    [[nodiscard]] blitzar_status Commit() noexcept;
    void Abort() noexcept;

    [[nodiscard]] const std::filesystem::path& TemporaryPath() const noexcept;
    [[nodiscard]] const std::filesystem::path& FinalPath() const noexcept;

private:
    std::filesystem::path directory_;
    std::filesystem::path temporary_path_;
    std::filesystem::path final_path_;
    bool directory_created_{};
    bool prepared_{};
    bool committed_{};
};

} // namespace blitzar_io

#endif
