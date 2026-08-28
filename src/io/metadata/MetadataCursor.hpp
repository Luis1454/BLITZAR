#ifndef BLITZAR_IO_METADATA_METADATA_CURSOR_HPP
#define BLITZAR_IO_METADATA_METADATA_CURSOR_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace blitzar_io {

class MetadataCursor final {
public:
    explicit MetadataCursor(std::string_view source) noexcept;

    [[nodiscard]] bool Expect(std::string_view expected) noexcept;
    [[nodiscard]] bool ReadUnsigned(
        std::string_view prefix, std::string_view suffix, std::uint64_t& value) noexcept;
    [[nodiscard]] bool ReadReal(
        std::string_view prefix, std::string_view suffix, double& value) noexcept;
    [[nodiscard]] bool ReadBoolean(
        std::string_view prefix, std::string_view suffix, bool& value) noexcept;
    [[nodiscard]] bool ReadString(
        std::string_view prefix, std::string_view suffix, std::string& value);
    [[nodiscard]] bool AtEnd() const noexcept;

private:
    [[nodiscard]] bool Next(std::string_view& line) noexcept;
    [[nodiscard]] bool ReadValue(
        std::string_view prefix, std::string_view suffix, std::string_view& value) noexcept;

    std::string_view source_;
    std::size_t position_{};
};

} // namespace blitzar_io

#endif
