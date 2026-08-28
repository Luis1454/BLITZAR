#include "io/metadata/MetadataCursor.hpp"

#include "io/metadata/MetadataText.hpp"

#include <charconv>
#include <cmath>
#include <locale>
#include <new>
#include <sstream>
#include <system_error>

namespace blitzar_io {

MetadataCursor::MetadataCursor(std::string_view source) noexcept : source_(source) {}

bool MetadataCursor::Expect(std::string_view expected) noexcept
{
    std::string_view line;

    return Next(line) && line == expected;
}

bool MetadataCursor::ReadUnsigned(
    std::string_view prefix, std::string_view suffix, std::uint64_t& value) noexcept
{
    std::string_view text;

    if (!ReadValue(prefix, suffix, text)) {
        return false;
    }

    const auto result = std::from_chars(text.data(), text.data() + text.size(), value);

    return result.ec == std::errc{} && result.ptr == text.data() + text.size();
}

bool MetadataCursor::ReadReal(
    std::string_view prefix, std::string_view suffix, double& value) noexcept
{
    std::string_view text;

    if (!ReadValue(prefix, suffix, text)) {
        return false;
    }

    try {
        std::istringstream input{std::string(text)};

        input.imbue(std::locale::classic());

        input >> std::noskipws;

        double parsed{};

        input >> parsed;

        if (input.fail() || !std::isfinite(parsed) ||
            input.peek() != std::char_traits<char>::eof()) {
            return false;
        }

        value = parsed;

        return true;
    }
    catch (const std::bad_alloc&) {
        return false;
    }
}

bool MetadataCursor::ReadBoolean(
    std::string_view prefix, std::string_view suffix, bool& value) noexcept
{
    std::string_view text;

    if (!ReadValue(prefix, suffix, text)) {
        return false;
    }

    if (text == "true") {
        value = true;

        return true;
    }

    if (text == "false") {
        value = false;

        return true;
    }

    return false;
}

bool MetadataCursor::ReadString(
    std::string_view prefix, std::string_view suffix, std::string& value)
{
    std::string_view text;

    return ReadValue(prefix, suffix, text) && DecodeMetadataString(text, value);
}

bool MetadataCursor::AtEnd() const noexcept
{
    return position_ == source_.size();
}

bool MetadataCursor::Next(std::string_view& line) noexcept
{
    if (position_ > source_.size()) {
        return false;
    }

    const std::size_t end = source_.find('\n', position_);
    const std::size_t limit = end == std::string_view::npos ? source_.size() : end;

    line = source_.substr(position_, limit - position_);

    if (!line.empty() && line.back() == '\r') {
        line.remove_suffix(1);
    }

    position_ = end == std::string_view::npos ? source_.size() : end + 1U;

    return true;
}

bool MetadataCursor::ReadValue(
    std::string_view prefix, std::string_view suffix, std::string_view& value) noexcept
{
    std::string_view line;

    if (!Next(line) || line.size() < prefix.size() + suffix.size() ||
        line.substr(0, prefix.size()) != prefix ||
        line.substr(line.size() - suffix.size()) != suffix) {
        return false;
    }

    const std::size_t value_size = line.size() - prefix.size() - suffix.size();

    value = line.substr(prefix.size(), value_size);

    return true;
}

} // namespace blitzar_io
