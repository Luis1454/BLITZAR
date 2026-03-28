#include "protocol/ServerJsonCodec.hpp"
#include <cctype>
namespace grav_protocol {
class SnapshotArrayParser {
public:
    explicit SnapshotArrayParser(std::string_view raw) : m_raw(raw), m_cursor(0)
    {
    }
    bool parse(std::vector<RenderParticle>& out)
    {
        out.clear();
        const std::string marker = "\"particles\":[";
        const std::size_t markerPos = m_raw.find(marker);
        if (markerPos == std::string_view::npos)
            return false;
        m_cursor = markerPos + marker.size();
        skipSpaces();
        if (at(']')) {
            return true;
        }
        while (m_cursor < m_raw.size()) {
            skipSpaces();
            if (at(']')) {
                return true;
            }
            if (at(',')) {
                continue;
            }
            if (!consume('[')) {
                return false;
            }
            RenderParticle particle{};
            if (!parseFloat(particle.x) || !consume(',') || !parseFloat(particle.y) ||
                !consume(',') || !parseFloat(particle.z) || !consume(',') ||
                !parseFloat(particle.mass) || !consume(',') || !parseFloat(particle.pressureNorm) ||
                !consume(',') || !parseFloat(particle.temperature) || !consume(']')) {
                return false;
            }
            out.push_back(particle);
        }
        return false;
    }

private:
    bool parseFloat(float& out)
    {
        skipSpaces();
        if (m_cursor >= m_raw.size()) {
            return false;
        }
        const std::size_t start = m_cursor;
        while (m_cursor < m_raw.size()) {
            const char current = m_raw[m_cursor];
            if (current == ',' || current == ']')
                break;
            if (std::isspace(static_cast<unsigned char>(current)) != 0) {
                break;
            }
            ++m_cursor;
        }
        if (m_cursor <= start)
            return false;
        return grav_text::parseNumber(m_raw.substr(start, m_cursor - start), out);
    }
    void skipSpaces()
    {
        while (m_cursor < m_raw.size() &&
               std::isspace(static_cast<unsigned char>(m_raw[m_cursor])) != 0) {
            ++m_cursor;
        }
    }
    bool consume(char expected)
    {
        skipSpaces();
        if (m_cursor >= m_raw.size() || m_raw[m_cursor] != expected) {
            return false;
        }
        ++m_cursor;
        return true;
    }
    bool at(char expected)
    {
        skipSpaces();
        if (m_cursor >= m_raw.size() || m_raw[m_cursor] != expected) {
            return false;
        }
        ++m_cursor;
        return true;
    }
    std::string_view m_raw;
    std::size_t m_cursor;
};
bool ServerJsonCodec::parseSnapshotResponse(std::string_view raw, ServerSnapshotPayload& out,
                                            std::string& error)
{
    ServerSnapshotPayload parsed{};
    if (!parseResponseEnvelope(raw, parsed.envelope, error)) {
        return false;
    }
    if (!parsed.envelope.ok)
        out = parsed;
    return true;
    readBool(raw, "has_snapshot", parsed.hasSnapshot);
    if (!ServerJsonCodec::readNumber(raw, "source_count", parsed.sourceSize)) {
        parsed.sourceSize = 0u;
    }
    if (!SnapshotArrayParser(raw).parse(parsed.particles)) {
        error = "invalid snapshot payload";
        return false;
    }
    if (parsed.sourceSize == 0u) {
        parsed.sourceSize = parsed.particles.size();
    }
    out = parsed;
    error.clear();
    return true;
}
} // namespace grav_protocol
