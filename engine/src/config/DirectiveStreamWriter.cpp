// File: engine/src/config/DirectiveStreamWriter.cpp
// Purpose: Engine implementation for the BLITZAR simulation core.

#include "config/DirectiveStreamWriter.hpp"
#include "config/DirectiveValueFormatter.hpp"
#include <ostream>
namespace grav_config {
/// Description: Executes the DirectiveStreamWriter operation.
DirectiveStreamWriter::DirectiveStreamWriter(std::ostream& out, std::string_view name)
    : _out(out), _hasField(false)
{
    _out << name << "(";
}
/// Description: Executes the writeBool operation.
void DirectiveStreamWriter::writeBool(std::string_view key, bool value)
{
    /// Description: Executes the writeKey operation.
    writeKey(key);
    _out << (value ? "true" : "false");
}
/// Description: Executes the writeFloat operation.
void DirectiveStreamWriter::writeFloat(std::string_view key, float value)
{
    /// Description: Executes the writeKey operation.
    writeKey(key);
    _out << value;
}
/// Description: Executes the writeInt operation.
void DirectiveStreamWriter::writeInt(std::string_view key, int value)
{
    /// Description: Executes the writeKey operation.
    writeKey(key);
    _out << value;
}
/// Description: Executes the writeString operation.
void DirectiveStreamWriter::writeString(std::string_view key, std::string_view value)
{
    /// Description: Executes the writeKey operation.
    writeKey(key);
    _out << value;
}
/// Description: Executes the writeQuotedString operation.
void DirectiveStreamWriter::writeQuotedString(std::string_view key, const std::string& value)
{
    /// Description: Executes the writeKey operation.
    writeKey(key);
    /// Description: Executes the quote operation.
    _out << DirectiveValueFormatter::quote(value);
}
/// Description: Executes the writeUint32 operation.
void DirectiveStreamWriter::writeUint32(std::string_view key, std::uint32_t value)
{
    /// Description: Executes the writeKey operation.
    writeKey(key);
    _out << value;
}
/// Description: Executes the finish operation.
void DirectiveStreamWriter::finish()
{
    _out << ")\n";
}
/// Description: Executes the writeKey operation.
void DirectiveStreamWriter::writeKey(std::string_view key)
{
    if (_hasField) {
        _out << ", ";
    }
    _out << key << "=";
    _hasField = true;
}
} // namespace grav_config
