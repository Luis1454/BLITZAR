// File: engine/include/config/DirectiveStreamWriter.hpp
// Purpose: Engine implementation for the BLITZAR simulation core.

#ifndef GRAVITY_ENGINE_INCLUDE_CONFIG_DIRECTIVESTREAMWRITER_HPP_
#define GRAVITY_ENGINE_INCLUDE_CONFIG_DIRECTIVESTREAMWRITER_HPP_
#include <cstdint>
#include <iosfwd>
#include <string>
#include <string_view>
namespace grav_config {
/// Description: Defines the DirectiveStreamWriter data or behavior contract.
class DirectiveStreamWriter final {
public:
    /// Description: Executes the DirectiveStreamWriter operation.
    DirectiveStreamWriter(std::ostream& out, std::string_view name);
    /// Description: Executes the writeBool operation.
    void writeBool(std::string_view key, bool value);
    /// Description: Executes the writeFloat operation.
    void writeFloat(std::string_view key, float value);
    /// Description: Executes the writeInt operation.
    void writeInt(std::string_view key, int value);
    /// Description: Executes the writeString operation.
    void writeString(std::string_view key, std::string_view value);
    /// Description: Executes the writeQuotedString operation.
    void writeQuotedString(std::string_view key, const std::string& value);
    /// Description: Executes the writeUint32 operation.
    void writeUint32(std::string_view key, std::uint32_t value);
    /// Description: Executes the finish operation.
    void finish();

private:
    /// Description: Executes the writeKey operation.
    void writeKey(std::string_view key);
    std::ostream& _out;
    bool _hasField;
};
} // namespace grav_config
#endif // GRAVITY_ENGINE_INCLUDE_CONFIG_DIRECTIVESTREAMWRITER_HPP_
