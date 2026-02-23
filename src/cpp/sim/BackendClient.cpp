#include "sim/BackendClient.hpp"
#include "sim/BackendProtocol.hpp"
#include "sim/SocketPlatform.hpp"
#include "sim/TextParse.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <exception>
#include <string>
#include <string_view>

namespace {

sim::socket::ConstBytes asBytes(std::string_view text)
{
    return sim::socket::ConstBytes{
        reinterpret_cast<const std::byte *>(text.data()),
        text.size()};
}

bool sendAll(sim::socket::Handle socketHandle, sim::socket::ConstBytes bytes)
{
    std::size_t offset = 0;
    while (offset < bytes.size) {
        const int sent = sim::socket::sendBytes(
            socketHandle,
            bytes.subview(offset));
        if (sent <= 0) {
            return false;
        }
        offset += static_cast<std::size_t>(sent);
    }
    return true;
}

bool findJsonValueStart(const std::string &request, const std::string &key, std::size_t &start)
{
    const std::string pattern = "\"" + key + "\"";
    const std::size_t keyPos = request.find(pattern);
    if (keyPos == std::string::npos) {
        return false;
    }
    const std::size_t colonPos = request.find(':', keyPos + pattern.size());
    if (colonPos == std::string::npos) {
        return false;
    }
    std::size_t cursor = colonPos + 1;
    while (cursor < request.size() && std::isspace(static_cast<unsigned char>(request[cursor])) != 0) {
        ++cursor;
    }
    if (cursor >= request.size()) {
        return false;
    }
    start = cursor;
    return true;
}

int clampTimeoutMs(int timeoutMs)
{
    return sim::socket::clampTimeoutMs(timeoutMs);
}

} // namespace

BackendClient::BackendClient()
    : _socket(sim::socket::invalidHandle()),
      _socketTimeoutMs(3000),
      _networkInitialized(false),
      _recvBuffer()
{
}

BackendClient::~BackendClient()
{
    disconnect();
}

bool BackendClient::connect(const std::string &host, std::uint16_t port)
{
    try {
        disconnect();

        if (!sim::socket::initializeSocketLayer()) {
            return false;
        }
        _networkInitialized = true;

        const sim::socket::Handle socketHandle = sim::socket::createTcpSocket();
        if (!sim::socket::isValid(socketHandle)) {
            disconnect();
            return false;
        }

        if (!sim::socket::connectIpv4(socketHandle, host, port, _socketTimeoutMs)) {
            sim::socket::closeSocket(socketHandle);
            disconnect();
            return false;
        }

        sim::socket::setSocketTimeoutMs(socketHandle, _socketTimeoutMs);

        _socket = socketHandle;
        _recvBuffer.clear();
        return true;
    } catch (...) {
        disconnect();
        return false;
    }
}

void BackendClient::setSocketTimeoutMs(int timeoutMs)
{
    _socketTimeoutMs = clampTimeoutMs(timeoutMs);
    if (isConnected()) {
        sim::socket::setSocketTimeoutMs(_socket, _socketTimeoutMs);
    }
}

int BackendClient::socketTimeoutMs() const
{
    return _socketTimeoutMs;
}

void BackendClient::disconnect()
{
    try {
        sim::socket::closeSocket(_socket);
        _socket = sim::socket::invalidHandle();
        _recvBuffer.clear();
        if (_networkInitialized) {
            sim::socket::shutdownSocketLayer();
            _networkInitialized = false;
        }
    } catch (...) {
        _socket = sim::socket::invalidHandle();
        _recvBuffer.clear();
        _networkInitialized = false;
    }
}

bool BackendClient::isConnected() const
{
    return sim::socket::isValid(_socket);
}

std::string BackendClient::trim(const std::string &value)
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

std::string BackendClient::toLower(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

std::string BackendClient::jsonEscape(const std::string &value)
{
    std::string escaped;
    escaped.reserve(value.size() + 8);
    for (unsigned char c : value) {
        switch (c) {
            case '\\':
                escaped += "\\\\";
                break;
            case '"':
                escaped += "\\\"";
                break;
            case '\n':
                escaped += "\\n";
                break;
            case '\r':
                escaped += "\\r";
                break;
            case '\t':
                escaped += "\\t";
                break;
            default:
                escaped.push_back(static_cast<char>(c));
                break;
        }
    }
    return escaped;
}

bool BackendClient::extractJsonString(const std::string &request, const std::string &key, std::string &out)
{
    std::size_t cursor = 0;
    if (!findJsonValueStart(request, key, cursor) || request[cursor] != '"') {
        return false;
    }
    ++cursor;
    std::string value;
    while (cursor < request.size()) {
        const char c = request[cursor++];
        if (c == '"') {
            out = value;
            return true;
        }
        if (c == '\\') {
            if (cursor >= request.size()) {
                return false;
            }
            const char escaped = request[cursor++];
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
        } else {
            value.push_back(c);
        }
    }
    return false;
}

bool BackendClient::extractJsonToken(const std::string &request, const std::string &key, std::string &out)
{
    std::size_t cursor = 0;
    if (!findJsonValueStart(request, key, cursor)) {
        return false;
    }
    const std::size_t end = request.find_first_of(",}", cursor);
    const std::size_t tokenEnd = (end == std::string::npos) ? request.size() : end;
    out = trim(request.substr(cursor, tokenEnd - cursor));
    return !out.empty();
}

bool BackendClient::extractJsonBool(const std::string &request, const std::string &key, bool &out)
{
    std::string token;
    if (!extractJsonToken(request, key, token)) {
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

template <typename NumberType>
bool BackendClient::extractJsonNumber(const std::string &request, const std::string &key, NumberType &out)
{
    std::string token;
    if (!extractJsonToken(request, key, token)) {
        return false;
    }
    return sim::text::parseNumber(token, out);
}

bool BackendClient::readLine(std::string &outLine)
{
    try {
        if (!sim::socket::isValid(_socket)) {
            return false;
        }

        while (true) {
            const std::size_t pos = _recvBuffer.find('\n');
            if (pos != std::string::npos) {
                outLine = _recvBuffer.substr(0, pos);
                _recvBuffer.erase(0, pos + 1);
                if (!outLine.empty() && outLine.back() == '\r') {
                    outLine.pop_back();
                }
                return true;
            }

            std::array<char, 2048> chunk{};
            const int received = sim::socket::recvBytes(
                _socket,
                sim::socket::MutableBytes{
                    reinterpret_cast<std::byte *>(chunk.data()),
                    chunk.size()});
            if (received <= 0) {
                return false;
            }
            _recvBuffer.append(chunk.data(), static_cast<std::size_t>(received));
            if (_recvBuffer.size() > (512u * 1024u)) {
                return false;
            }
        }
    } catch (...) {
        return false;
    }
}

bool extractSnapshotParticles(const std::string &raw, std::vector<RenderParticle> &outSnapshot)
{
    outSnapshot.clear();
    const std::string marker = "\"particles\":[";
    const std::size_t markerPos = raw.find(marker);
    if (markerPos == std::string::npos) {
        return false;
    }
    std::size_t i = markerPos + marker.size();
    if (i >= raw.size()) {
        return false;
    }
    while (i < raw.size() && std::isspace(static_cast<unsigned char>(raw[i])) != 0) {
        ++i;
    }
    if (i < raw.size() && raw[i] == ']') {
        return true;
    }

    auto skipSpaces = [&]() {
        while (i < raw.size() && std::isspace(static_cast<unsigned char>(raw[i])) != 0) {
            ++i;
        }
    };
    auto parseFloat = [&](float &out) -> bool {
        skipSpaces();
        if (i >= raw.size()) {
            return false;
        }
        const std::size_t start = i;
        while (i < raw.size()) {
            const char c = raw[i];
            if (c == ',' || c == ']') {
                break;
            }
            if (std::isspace(static_cast<unsigned char>(c)) != 0) {
                break;
            }
            ++i;
        }
        if (i <= start) {
            return false;
        }
        return sim::text::parseNumber(std::string_view(raw.data() + start, i - start), out);
    };

    while (i < raw.size()) {
        skipSpaces();
        if (i >= raw.size()) {
            break;
        }
        if (raw[i] == ']') {
            return true;
        }
        if (raw[i] == ',') {
            ++i;
            continue;
        }
        if (raw[i] != '[') {
            return false;
        }
        ++i;
        RenderParticle p{};
        if (!parseFloat(p.x)) return false;
        skipSpaces(); if (i >= raw.size() || raw[i] != ',') return false; ++i;
        if (!parseFloat(p.y)) return false;
        skipSpaces(); if (i >= raw.size() || raw[i] != ',') return false; ++i;
        if (!parseFloat(p.z)) return false;
        skipSpaces(); if (i >= raw.size() || raw[i] != ',') return false; ++i;
        if (!parseFloat(p.mass)) return false;
        skipSpaces(); if (i >= raw.size() || raw[i] != ',') return false; ++i;
        if (!parseFloat(p.pressureNorm)) return false;
        skipSpaces(); if (i >= raw.size() || raw[i] != ',') return false; ++i;
        if (!parseFloat(p.temperature)) return false;
        skipSpaces();
        if (i >= raw.size() || raw[i] != ']') {
            return false;
        }
        ++i;
        outSnapshot.push_back(p);
    }
    return false;
}

BackendClientResponse BackendClient::sendJson(const std::string &jsonLine)
{
    BackendClientResponse response;
    try {
        if (!isConnected()) {
            response.error = "not connected";
            return response;
        }

        std::string line = trim(jsonLine);
        if (line.empty()) {
            response.error = "empty request";
            return response;
        }
        line.push_back('\n');

        if (!sendAll(_socket, asBytes(line))) {
            response.error = "send failed";
            disconnect();
            return response;
        }

        if (!readLine(response.raw)) {
            response.error = "recv failed";
            disconnect();
            return response;
        }

        bool ok = false;
        if (extractJsonBool(response.raw, "ok", ok)) {
            response.ok = ok;
        } else {
            response.error = "invalid response";
            return response;
        }

        if (!response.ok) {
            std::string errorMessage;
            if (extractJsonString(response.raw, "error", errorMessage)) {
                response.error = errorMessage;
            } else {
                response.error = "backend error";
            }
        }
        return response;
    } catch (const std::exception &ex) {
        response.ok = false;
        response.error = ex.what();
        disconnect();
        return response;
    } catch (...) {
        response.ok = false;
        response.error = "unknown sendJson error";
        disconnect();
        return response;
    }
}

BackendClientResponse BackendClient::sendCommand(const std::string &cmd, const std::string &fieldsJson)
{
    std::string payload = std::string("{\"cmd\":\"") + jsonEscape(cmd) + "\"";
    if (!fieldsJson.empty()) {
        payload += ",";
        payload += fieldsJson;
    }
    payload += "}";
    return sendJson(payload);
}

BackendClientResponse BackendClient::getStatus(BackendClientStatus &outStatus)
{
    try {
        BackendClientResponse response = sendCommand(std::string(sim::protocol::cmd::Status));
        if (!response.ok) {
            return response;
        }

        BackendClientStatus parsed{};
        extractJsonNumber(response.raw, "steps", parsed.steps);
        extractJsonNumber(response.raw, "dt", parsed.dt);
        extractJsonBool(response.raw, "paused", parsed.paused);
        extractJsonBool(response.raw, "faulted", parsed.faulted);
        extractJsonNumber(response.raw, "fault_step", parsed.faultStep);
        extractJsonString(response.raw, "fault_reason", parsed.faultReason);
        extractJsonBool(response.raw, "sph", parsed.sphEnabled);
        extractJsonNumber(response.raw, "backend_fps", parsed.backendFps);
        extractJsonNumber(response.raw, "particles", parsed.particleCount);
        extractJsonString(response.raw, "solver", parsed.solver);
        extractJsonNumber(response.raw, "ekin", parsed.kineticEnergy);
        extractJsonNumber(response.raw, "epot", parsed.potentialEnergy);
        extractJsonNumber(response.raw, "eth", parsed.thermalEnergy);
        extractJsonNumber(response.raw, "erad", parsed.radiatedEnergy);
        extractJsonNumber(response.raw, "etot", parsed.totalEnergy);
        extractJsonNumber(response.raw, "drift_pct", parsed.energyDriftPct);
        extractJsonBool(response.raw, "estimated", parsed.energyEstimated);

        outStatus = parsed;
        return response;
    } catch (const std::exception &ex) {
        BackendClientResponse response;
        response.error = ex.what();
        return response;
    } catch (...) {
        BackendClientResponse response;
        response.error = "unknown getStatus error";
        return response;
    }
}

BackendClientResponse BackendClient::getSnapshot(std::vector<RenderParticle> &outSnapshot, std::uint32_t maxPoints)
{
    try {
        maxPoints = sim::protocol::clampSnapshotPoints(maxPoints);
        BackendClientResponse response = sendCommand(
            std::string(sim::protocol::cmd::GetSnapshot),
            "\"max_points\":" + std::to_string(maxPoints));
        if (!response.ok) {
            return response;
        }
        if (!extractSnapshotParticles(response.raw, outSnapshot)) {
            response.ok = false;
            response.error = "invalid snapshot payload";
        }
        return response;
    } catch (const std::exception &ex) {
        BackendClientResponse response;
        response.error = ex.what();
        return response;
    } catch (...) {
        BackendClientResponse response;
        response.error = "unknown getSnapshot error";
        return response;
    }
}
