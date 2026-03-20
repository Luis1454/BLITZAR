#include "config/DirectiveStreamWriter.hpp"

#include "config/DirectiveValueFormatter.hpp"

#include <ostream>

namespace grav_config {

DirectiveStreamWriter::DirectiveStreamWriter(std::ostream &out, std::string_view name)
    : _out(out), _hasField(false)
{
    _out << name << "(";
}

void DirectiveStreamWriter::writeBool(std::string_view key, bool value)
{
    writeKey(key);
    _out << (value ? "true" : "false");
}

void DirectiveStreamWriter::writeFloat(std::string_view key, float value)
{
    writeKey(key);
    _out << value;
}

void DirectiveStreamWriter::writeInt(std::string_view key, int value)
{
    writeKey(key);
    _out << value;
}

void DirectiveStreamWriter::writeString(std::string_view key, std::string_view value)
{
    writeKey(key);
    _out << value;
}

void DirectiveStreamWriter::writeQuotedString(std::string_view key, const std::string &value)
{
    writeKey(key);
    _out << DirectiveValueFormatter::quote(value);
}

void DirectiveStreamWriter::writeUint32(std::string_view key, std::uint32_t value)
{
    writeKey(key);
    _out << value;
}

void DirectiveStreamWriter::finish()
{
    _out << ")\n";
}

void DirectiveStreamWriter::writeKey(std::string_view key)
{
    if (_hasField) {
        _out << ", ";
    }
    _out << key << "=";
    _hasField = true;
}

} // namespace grav_config
