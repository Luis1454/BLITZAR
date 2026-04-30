/*
 * @file engine/include/config/DirectiveStreamWriter.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Public configuration interfaces and validation contracts for simulation setup.
 */

#ifndef BLITZAR_ENGINE_INCLUDE_CONFIG_DIRECTIVESTREAMWRITER_HPP_
#define BLITZAR_ENGINE_INCLUDE_CONFIG_DIRECTIVESTREAMWRITER_HPP_
#include <cstdint>
#include <iosfwd>
#include <string>
#include <string_view>

namespace bltzr_config {
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
} // namespace bltzr_config
#endif // BLITZAR_ENGINE_INCLUDE_CONFIG_DIRECTIVESTREAMWRITER_HPP_
