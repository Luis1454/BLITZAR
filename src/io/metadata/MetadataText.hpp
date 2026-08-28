#ifndef BLITZAR_IO_METADATA_METADATA_TEXT_HPP
#define BLITZAR_IO_METADATA_METADATA_TEXT_HPP

#include <string>
#include <string_view>

namespace blitzar_io {

[[nodiscard]] bool DecodeMetadataString(std::string_view text, std::string& value);

} // namespace blitzar_io

#endif
