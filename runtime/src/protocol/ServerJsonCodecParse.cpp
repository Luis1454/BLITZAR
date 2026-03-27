#include "protocol/ServerJsonCodec.hpp"

#include <algorithm>
#include <cctype>

namespace grav_protocol {

bool ServerJsonCodec::parseCommandRequest(std::string_view raw, ServerCommandRequest &out, std::string &error)
{
    const std::string trimmedRaw = trim(raw);
    if (trimmedRaw.empty() || trimmedRaw.front() != '{' || trimmedRaw.back() != '}') {
        error = "invalid json";
        return false;
    }

    ServerCommandRequest parsed{};
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
    out = parsed;
    error.clear();
    return true;
}

bool ServerJsonCodec::parseResponseEnvelope(std::string_view raw, ServerResponseEnvelope &out, std::string &error)
{
    const std::string trimmedRaw = trim(raw);
    if (trimmedRaw.empty() || trimmedRaw.front() != '{' || trimmedRaw.back() != '}') {
        error = "invalid json";
        return false;
    }

    ServerResponseEnvelope parsed{};
    if (!readBool(raw, "ok", parsed.ok)) {
        error = "invalid response";
        return false;
    }
    readString(raw, "cmd", parsed.cmd);
    if (!parsed.ok && !readString(raw, "error", parsed.error)) {
        parsed.error = "server error";
    }
    out = parsed;
    error.clear();
    return true;
}

bool ServerJsonCodec::readString(std::string_view raw, std::string_view key, std::string &out)
{
    std::size_t cursor = 0;
    if (!findValueStart(raw, key, cursor) || raw[cursor] != '"') {
        return false;
    }
    ++cursor;
    std::string value;
    value.reserve(32u);
    while (cursor < raw.size()) {
        const char current = raw[cursor++];
        if (current == '"') {
            out = value;
            return true;
        }
        if (current == '\\') {
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
        value.push_back(current);
    }
    return false;
}

bool ServerJsonCodec::readBool(std::string_view raw, std::string_view key, bool &out)
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
    if (token == "false") {
        out = false;
        return true;
    }
    return false;
}

std::string ServerJsonCodec::trim(std::string_view value)
{
    const auto begin = std::find_if_not(value.begin(), value.end(), [](unsigned char current) {
        return std::isspace(current) != 0;
    });
    const auto end = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char current) {
        return std::isspace(current) != 0;
    }).base();
    if (begin >= end) {
        return {};
    }
    return std::string(begin, end);
}

std::string ServerJsonCodec::toLower(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char current) {
        return static_cast<char>(std::tolower(current));
    });
    return value;
}

bool ServerJsonCodec::findValueStart(std::string_view raw, std::string_view key, std::size_t &start)
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
    start = cursor;
    return true;
}

bool ServerJsonCodec::readToken(std::string_view raw, std::string_view key, std::string &out)
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
