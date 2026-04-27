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
    /// Description: Describes the directive stream writer operation contract.
    DirectiveStreamWriter(std::ostream& out, std::string_view name);
    /// Description: Describes the write bool operation contract.
    void writeBool(std::string_view key, bool value);
    /// Description: Describes the write float operation contract.
    void writeFloat(std::string_view key, float value);
    /// Description: Describes the write int operation contract.
    void writeInt(std::string_view key, int value);
    /// Description: Describes the write string operation contract.
    void writeString(std::string_view key, std::string_view value);
    /// Description: Describes the write quoted string operation contract.
    void writeQuotedString(std::string_view key, const std::string& value);
    /// Description: Describes the write uint32 operation contract.
    void writeUint32(std::string_view key, std::uint32_t value);
    /// Description: Describes the finish operation contract.
    void finish();

private:
    /// Description: Describes the write key operation contract.
    void writeKey(std::string_view key);
    std::ostream& _out;
    bool _hasField;
};
} // namespace grav_config
#endif // GRAVITY_ENGINE_INCLUDE_CONFIG_DIRECTIVESTREAMWRITER_HPP_
