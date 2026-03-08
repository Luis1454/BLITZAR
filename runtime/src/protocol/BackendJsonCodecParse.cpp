#include "protocol/BackendJsonCodec.hpp"

#include <algorithm>
#include <cctype>

namespace grav_protocol {

class SnapshotArrayParser {
    public:
        explicit SnapshotArrayParser(std::string_view raw) : _raw(raw), _cursor(0) {}

        bool parse(std::vector<RenderParticle> &out)
        {
            out.clear();
            const std::string marker = "\"particles\":[";
            const std::size_t markerPos = _raw.find(marker);
            if (markerPos == std::string_view::npos) {
                return false;
            }
            _cursor = markerPos + marker.size();
            skipSpaces();
            if (at(']')) {
                return true;
            }
            while (_cursor < _raw.size()) {
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
                if (!parseFloat(particle.x)
                    || !consume(',')
                    || !parseFloat(particle.y)
                    || !consume(',')
                    || !parseFloat(particle.z)
                    || !consume(',')
                    || !parseFloat(particle.mass)
                    || !consume(',')
                    || !parseFloat(particle.pressureNorm)
                    || !consume(',')
                    || !parseFloat(particle.temperature)
                    || !consume(']')) {
                    return false;
                }
                out.push_back(particle);
            }
            return false;
        }

    private:
        bool parseFloat(float &out)
        {
            skipSpaces();
            if (_cursor >= _raw.size()) {
                return false;
            }
            const std::size_t start = _cursor;
            while (_cursor < _raw.size()) {
                const char c = _raw[_cursor];
                if (c == ',' || c == ']') {
                    break;
                }
                if (std::isspace(static_cast<unsigned char>(c)) != 0) {
                    break;
                }
                ++_cursor;
            }
            if (_cursor <= start) {
                return false;
            }
            return grav_text::parseNumber(_raw.substr(start, _cursor - start), out);
        }

        void skipSpaces()
        {
            while (_cursor < _raw.size() && std::isspace(static_cast<unsigned char>(_raw[_cursor])) != 0) {
                ++_cursor;
            }
        }

        bool consume(char expected)
        {
            skipSpaces();
            if (_cursor >= _raw.size() || _raw[_cursor] != expected) {
                return false;
            }
            ++_cursor; return true;
        }

        bool at(char expected)
        {
            skipSpaces();
            if (_cursor >= _raw.size() || _raw[_cursor] != expected) {
                return false;
            }
            ++_cursor; return true;
        }

        std::string_view _raw;
        std::size_t _cursor;
};

bool BackendJsonCodec::parseCommandRequest(std::string_view raw, BackendCommandRequest &out, std::string &error)
{
    BackendCommandRequest parsed{};
    if (!readString(raw, "cmd", parsed.cmd)) {
        error = "missing cmd";
        return false;
    }
    readString(raw, "token", parsed.token);
    parsed.cmd = toLower(trim(parsed.cmd));
    if (parsed.cmd.empty()) {
        error = "missing cmd";
        return false;
    }
    out = parsed; error.clear(); return true;
}

bool BackendJsonCodec::parseResponseEnvelope(std::string_view raw, BackendResponseEnvelope &out, std::string &error)
{
    BackendResponseEnvelope parsed{};
    if (!readBool(raw, "ok", parsed.ok)) {
        error = "invalid response";
        return false;
    }
    readString(raw, "cmd", parsed.cmd);
    if (!parsed.ok && !readString(raw, "error", parsed.error)) {
        parsed.error = "backend error";
    }
    out = parsed; error.clear(); return true;
}

bool BackendJsonCodec::parseStatusResponse(std::string_view raw, BackendStatusPayload &out, std::string &error)
{
    BackendStatusPayload parsed{};
    if (!parseResponseEnvelope(raw, parsed.envelope, error)) {
        return false;
    }
    if (!parsed.envelope.ok) {
        out = parsed;
        return true;
    }
    readNumber(raw, "steps", parsed.steps);
    readNumber(raw, "dt", parsed.dt);
    readBool(raw, "paused", parsed.paused);
    readBool(raw, "faulted", parsed.faulted);
    readNumber(raw, "fault_step", parsed.faultStep);
    readString(raw, "fault_reason", parsed.faultReason);
    readBool(raw, "sph", parsed.sphEnabled);
    readNumber(raw, "backend_fps", parsed.backendFps);
    readNumber(raw, "particles", parsed.particleCount);
    readString(raw, "solver", parsed.solver);
    readString(raw, "integrator", parsed.integrator);
    readNumber(raw, "ekin", parsed.kineticEnergy);
    readNumber(raw, "epot", parsed.potentialEnergy);
    readNumber(raw, "eth", parsed.thermalEnergy);
    readNumber(raw, "erad", parsed.radiatedEnergy);
    readNumber(raw, "etot", parsed.totalEnergy);
    readNumber(raw, "drift_pct", parsed.energyDriftPct);
    readBool(raw, "estimated", parsed.energyEstimated);
    out = parsed; error.clear(); return true;
}

bool BackendJsonCodec::parseSnapshotResponse(std::string_view raw, BackendSnapshotPayload &out, std::string &error)
{
    BackendSnapshotPayload parsed{};
    if (!parseResponseEnvelope(raw, parsed.envelope, error)) {
        return false;
    }
    if (!parsed.envelope.ok) {
        out = parsed;
        return true;
    }
    readBool(raw, "has_snapshot", parsed.hasSnapshot);
    if (!SnapshotArrayParser(raw).parse(parsed.particles)) {
        error = "invalid snapshot payload";
        return false;
    }
    out = parsed; error.clear(); return true;
}

bool BackendJsonCodec::readString(std::string_view raw, std::string_view key, std::string &out)
{
    std::size_t cursor = 0;
    if (!findValueStart(raw, key, cursor) || raw[cursor] != '"') {
        return false;
    }
    ++cursor;
    std::string value;
    value.reserve(32u);
    while (cursor < raw.size()) {
        const char c = raw[cursor++];
        if (c == '"') {
            out = value;
            return true;
        }
        if (c == '\\') {
            if (cursor >= raw.size()) {
                return false;
            }
            const char escaped = raw[cursor++];
            switch (escaped) {
                case 'n':
                    value.push_back('\n');
                    break;
                case 'r':
                    value.push_back('\r');
                    break;
                case 't':
                    value.push_back('\t');
                    break;
                default:
                    value.push_back(escaped);
                    break;
            }
            continue;
        }
        value.push_back(c);
    }
    return false;
}

bool BackendJsonCodec::readBool(std::string_view raw, std::string_view key, bool &out)
{
    std::string token;
    if (!readToken(raw, key, token)) {
        return false;
    }
    token = toLower(token);
    if (token == "true") {
        out = true;
        return true;
    }
    if (token == "false") { out = false; return true; }
    return false;
}

std::string BackendJsonCodec::trim(std::string_view value)
{
    const auto begin = std::find_if_not(value.begin(), value.end(), [](unsigned char c) {
        return std::isspace(c) != 0;
    });
    const auto end = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char c) {
        return std::isspace(c) != 0;
    }).base();
    if (begin >= end) {
        return {};
    }
    return std::string(begin, end);
}

std::string BackendJsonCodec::toLower(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

bool BackendJsonCodec::findValueStart(std::string_view raw, std::string_view key, std::size_t &start)
{
    const std::string pattern = "\"" + std::string(key) + "\"";
    const std::size_t keyPos = raw.find(pattern);
    if (keyPos == std::string_view::npos) {
        return false;
    }
    const std::size_t colonPos = raw.find(':', keyPos + pattern.size());
    if (colonPos == std::string_view::npos) {
        return false;
    }
    std::size_t cursor = colonPos + 1;
    while (cursor < raw.size() && std::isspace(static_cast<unsigned char>(raw[cursor])) != 0) {
        ++cursor;
    }
    if (cursor >= raw.size()) {
        return false;
    }
    start = cursor; return true;
}

bool BackendJsonCodec::readToken(std::string_view raw, std::string_view key, std::string &out)
{
    std::size_t cursor = 0;
    if (!findValueStart(raw, key, cursor)) {
        return false;
    }
    const std::size_t end = raw.find_first_of(",}", cursor);
    const std::size_t tokenEnd = (end == std::string_view::npos) ? raw.size() : end;
    out = trim(raw.substr(cursor, tokenEnd - cursor));
    return !out.empty();
}

} // namespace grav_protocol
