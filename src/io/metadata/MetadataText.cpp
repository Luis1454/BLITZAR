#include "io/metadata/MetadataText.hpp"

#include <cstddef>
#include <limits>

namespace blitzar_io {

namespace {

[[nodiscard]] int HexValue(char value) noexcept
{
    if (value >= '0' && value <= '9') {
        return value - '0';
    }

    if (value >= 'a' && value <= 'f') {
        return value - 'a' + 10;
    }

    if (value >= 'A' && value <= 'F') {
        return value - 'A' + 10;
    }

    return -1;
}

[[nodiscard]] bool AppendUnicodeEscape(
    std::string_view text, std::size_t& index, std::string& value)
{
    if (index + 4U >= text.size()) {
        return false;
    }

    int code = 0;

    for (std::size_t digit = 1; digit <= 4U; ++digit) {
        const int value_digit = HexValue(text[index + digit]);

        if (value_digit < 0) {
            return false;
        }

        code = (code << 4) | value_digit;
    }

    if (code > std::numeric_limits<unsigned char>::max()) {
        return false;
    }

    value.push_back(static_cast<char>(code));

    index += 4U;

    return true;
}

[[nodiscard]] bool AppendEscape(std::string_view text, std::size_t& index, std::string& value)
{
    if (++index + 1U >= text.size()) {
        return false;
    }

    const char escaped = text[index];

    switch (escaped) {
    case '"':
    case '\\':

        value.push_back(escaped);

        return true;

    case 'b':

        value.push_back('\b');

        return true;

    case 'f':

        value.push_back('\f');

        return true;

    case 'n':

        value.push_back('\n');

        return true;

    case 'r':

        value.push_back('\r');

        return true;

    case 't':

        value.push_back('\t');

        return true;

    case 'u':

        return AppendUnicodeEscape(text, index, value);

    default:

        return false;
    }
}

} // namespace

bool DecodeMetadataString(std::string_view text, std::string& value)
{
    if (text.size() < 2U || text.front() != '"' || text.back() != '"') {
        return false;
    }

    value.clear();

    for (std::size_t index = 1; index + 1U < text.size(); ++index) {
        const char character = text[index];

        if (character == '"' || static_cast<unsigned char>(character) < 0x20U) {
            return false;
        }

        if (character != '\\') {
            value.push_back(character);

            continue;
        }

        if (!AppendEscape(text, index, value)) {
            return false;
        }
    }

    return true;
}

} // namespace blitzar_io
