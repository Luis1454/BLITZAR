// File: engine/include/config/DirectiveStreamWriter.hpp
// Purpose: Engine implementation for the BLITZAR simulation core.

#ifndef GRAVITY_ENGINE_INCLUDE_CONFIG_DIRECTIVESTREAMWRITER_HPP_
#define GRAVITY_ENGINE_INCLUDE_CONFIG_DIRECTIVESTREAMWRITER_HPP_
#include <cstdint>
#include <iosfwd>
#include <string>
#include <string_view>
namespace grav_config {
class DirectiveStreamWriter final {
public:
    DirectiveStreamWriter(std::ostream& out, std::string_view name);
    void writeBool(std::string_view key, bool value);
    void writeFloat(std::string_view key, float value);
    void writeInt(std::string_view key, int value);
    void writeString(std::string_view key, std::string_view value);
    void writeQuotedString(std::string_view key, const std::string& value);
    void writeUint32(std::string_view key, std::uint32_t value);
    void finish();

private:
    void writeKey(std::string_view key);
    std::ostream& _out;
    bool _hasField;
};
} // namespace grav_config
#endif // GRAVITY_ENGINE_INCLUDE_CONFIG_DIRECTIVESTREAMWRITER_HPP_
