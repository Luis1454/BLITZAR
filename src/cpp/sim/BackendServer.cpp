#include "sim/BackendServer.hpp"
#include "sim/BackendProtocol.hpp"
#include "sim/SocketPlatform.hpp"
#include "sim/TextParse.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

namespace {

std::string trim(const std::string &input)
{
    const auto begin = std::find_if_not(input.begin(), input.end(), [](unsigned char c) {
        return std::isspace(c) != 0;
    });
    const auto end = std::find_if_not(input.rbegin(), input.rend(), [](unsigned char c) {
        return std::isspace(c) != 0;
    }).base();
    if (begin >= end) {
        return {};
    }
    return std::string(begin, end);
}

std::string toLower(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

std::string jsonEscape(const std::string &value)
{
    std::string escaped;
    escaped.reserve(value.size() + 8);
    for (unsigned char c : value) {
        switch (c) {
            case '\\':
                escaped += "\\\\";
                break;
            case '\"':
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

bool extractJsonString(const std::string &request, const std::string &key, std::string &out)
{
    std::size_t cursor = 0;
    if (!findJsonValueStart(request, key, cursor) || request[cursor] != '"') {
        return false;
    }
    ++cursor;
    std::string value;
    value.reserve(32);
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

bool extractJsonToken(const std::string &request, const std::string &key, std::string &out)
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

template <typename NumberType>
bool extractJsonNumber(const std::string &request, const std::string &key, NumberType &out)
{
    std::string token;
    if (!extractJsonToken(request, key, token)) {
        return false;
    }
    return sim::text::parseNumber(token, out);
}

bool extractJsonBool(const std::string &request, const std::string &key, bool &out)
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

std::string boolToJson(bool value)
{
    return value ? "true" : "false";
}

std::string makeOkResponse(const std::string &cmd)
{
    return std::string("{\"ok\":true,\"cmd\":\"") + jsonEscape(cmd) + "\"}";
}

std::string makeErrorResponse(const std::string &cmd, const std::string &message)
{
    return std::string("{\"ok\":false,\"cmd\":\"") + jsonEscape(cmd)
        + "\",\"error\":\"" + jsonEscape(message) + "\"}";
}

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

} // namespace

BackendServer::BackendServer(SimulationBackend &backend)
    : _backend(backend),
      _running(false),
      _shutdownRequested(false),
      _acceptThread(),
      _listenSocket(sim::socket::invalidHandle()),
      _bindAddress("127.0.0.1"),
      _port(0),
      _networkInitialized(false),
      _socketMutex(),
      _clientThreads()
{
}

BackendServer::~BackendServer()
{
    stop();
}

bool BackendServer::start(std::uint16_t port, const std::string &bindAddress)
{
    try {
        if (_running.load()) {
            return true;
        }

        if (!sim::socket::initializeSocketLayer()) {
            std::cerr << "[ipc] failed to initialize socket layer\n";
            return false;
        }
        _networkInitialized = true;

        const sim::socket::Handle listenSocket = sim::socket::createTcpSocket();
        if (!sim::socket::isValid(listenSocket)) {
            std::cerr << "[ipc] failed to create socket\n";
            stop();
            return false;
        }

        sim::socket::setReuseAddress(listenSocket, true);

        if (!sim::socket::bindIpv4(listenSocket, bindAddress, port)) {
            std::cerr << "[ipc] bind failed on " << bindAddress << ":" << port << "\n";
            sim::socket::closeSocket(listenSocket);
            stop();
            return false;
        }

        if (!sim::socket::listenSocket(listenSocket, 8)) {
            std::cerr << "[ipc] listen failed\n";
            sim::socket::closeSocket(listenSocket);
            stop();
            return false;
        }

        {
            std::lock_guard<std::mutex> lock(_socketMutex);
            _listenSocket = listenSocket;
            _bindAddress = bindAddress;
            _port = port;
        }
        _shutdownRequested.store(false);
        _running.store(true);
        _acceptThread = std::thread(&BackendServer::acceptLoop, this);
        return true;
    } catch (const std::exception &ex) {
        std::cerr << "[ipc] start exception: " << ex.what() << "\n";
        stop();
        return false;
    } catch (...) {
        std::cerr << "[ipc] start exception: unknown\n";
        stop();
        return false;
    }
}

void BackendServer::stop()
{
    try {
        _running.store(false);

        sim::socket::Handle listenSocket = sim::socket::invalidHandle();
        {
            std::lock_guard<std::mutex> lock(_socketMutex);
            listenSocket = _listenSocket;
            _listenSocket = sim::socket::invalidHandle();
        }
        sim::socket::closeSocket(listenSocket);

        if (_acceptThread.joinable()) {
            _acceptThread.join();
        }
        for (std::thread &thread : _clientThreads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        _clientThreads.clear();

        if (_networkInitialized) {
            sim::socket::shutdownSocketLayer();
            _networkInitialized = false;
        }
    } catch (const std::exception &ex) {
        std::cerr << "[ipc] stop exception: " << ex.what() << "\n";
    } catch (...) {
        std::cerr << "[ipc] stop exception: unknown\n";
    }
    _running.store(false);
    _listenSocket = sim::socket::invalidHandle();
    _clientThreads.clear();
    if (_networkInitialized) {
        try {
            sim::socket::shutdownSocketLayer();
        } catch (...) {
        }
        _networkInitialized = false;
    }
}

bool BackendServer::isRunning() const
{
    return _running.load();
}

bool BackendServer::shutdownRequested() const
{
    return _shutdownRequested.load();
}

void BackendServer::acceptLoop()
{
    try {
        while (_running.load()) {
            sim::socket::Handle listenSocket = sim::socket::invalidHandle();
            {
                std::lock_guard<std::mutex> lock(_socketMutex);
                listenSocket = _listenSocket;
            }
            if (!sim::socket::isValid(listenSocket)) {
                break;
            }

            if (!sim::socket::waitReadable(listenSocket, 200)) {
                continue;
            }

            const sim::socket::Handle clientSocket = sim::socket::acceptSocket(listenSocket);
            if (!sim::socket::isValid(clientSocket)) {
                continue;
            }

            sim::socket::setSocketTimeoutMs(clientSocket, 500);

            _clientThreads.emplace_back(&BackendServer::handleClient, this, clientSocket);
        }
    } catch (const std::exception &ex) {
        std::cerr << "[ipc] accept loop exception: " << ex.what() << "\n";
        _running.store(false);
    } catch (...) {
        std::cerr << "[ipc] accept loop exception: unknown\n";
        _running.store(false);
    }
}

void BackendServer::handleClient(SocketHandle client)
{
    try {
        const sim::socket::Handle clientSocket = client;
        std::string buffer;
        buffer.reserve(2048);
        std::array<char, 2048> chunk{};

        while (_running.load()) {
            const int received = sim::socket::recvBytes(
                clientSocket,
                sim::socket::MutableBytes{
                    reinterpret_cast<std::byte *>(chunk.data()),
                    chunk.size()});
            if (received <= 0) {
                if (received < 0 && sim::socket::wouldBlockOrTimeoutLastError() && _running.load()) {
                    continue;
                }
                break;
            }
            buffer.append(chunk.data(), static_cast<std::size_t>(received));

            while (true) {
                std::size_t newline = buffer.find('\n');
                if (newline == std::string::npos) {
                    break;
                }
                std::string request = trim(buffer.substr(0, newline));
                buffer.erase(0, newline + 1);
                if (!request.empty() && request.back() == '\r') {
                    request.pop_back();
                }
                if (request.empty()) {
                    continue;
                }

                const std::string response = processRequest(request) + "\n";
                if (!sendAll(clientSocket, asBytes(response))) {
                    sim::socket::closeSocket(clientSocket);
                    return;
                }
                if (_shutdownRequested.load()) {
                    sim::socket::closeSocket(clientSocket);
                    return;
                }
            }
        }

        sim::socket::closeSocket(clientSocket);
    } catch (const std::exception &ex) {
        std::cerr << "[ipc] client exception: " << ex.what() << "\n";
        sim::socket::closeSocket(static_cast<sim::socket::Handle>(client));
    } catch (...) {
        std::cerr << "[ipc] client exception: unknown\n";
        sim::socket::closeSocket(static_cast<sim::socket::Handle>(client));
    }
}

std::string BackendServer::processRequest(const std::string &request)
{
    try {
        std::string cmd;
        if (!extractJsonString(request, "cmd", cmd)) {
            return makeErrorResponse("unknown", "missing cmd");
        }
        cmd = toLower(trim(cmd));

        if (cmd == sim::protocol::cmd::Status) {
            const SimulationStats stats = _backend.getStats();
            std::ostringstream out;
            out << std::fixed << std::setprecision(6)
                << "{\"ok\":true,\"cmd\":\"" << sim::protocol::cmd::Status << "\""
                << ",\"steps\":" << stats.steps
                << ",\"dt\":" << stats.dt
                << ",\"paused\":" << boolToJson(stats.paused)
                << ",\"faulted\":" << boolToJson(stats.faulted)
                << ",\"fault_step\":" << stats.faultStep
                << ",\"fault_reason\":\"" << jsonEscape(stats.faultReason) << "\""
                << ",\"sph\":" << boolToJson(stats.sphEnabled)
                << ",\"backend_fps\":" << stats.backendFps
                << ",\"particles\":" << stats.particleCount
                << ",\"solver\":\"" << jsonEscape(stats.solverName) << "\""
                << ",\"ekin\":" << stats.kineticEnergy
                << ",\"epot\":" << stats.potentialEnergy
                << ",\"eth\":" << stats.thermalEnergy
                << ",\"erad\":" << stats.radiatedEnergy
                << ",\"etot\":" << stats.totalEnergy
                << ",\"drift_pct\":" << stats.energyDriftPct
                << ",\"estimated\":" << boolToJson(stats.energyEstimated)
                << "}";
            return out.str();
        }
        if (cmd == sim::protocol::cmd::GetSnapshot) {
            unsigned int maxPoints = sim::protocol::kSnapshotDefaultPoints;
            extractJsonNumber(request, "max_points", maxPoints);
            maxPoints = sim::protocol::clampSnapshotPoints(maxPoints);

            std::vector<RenderParticle> snapshot;
            const bool hasSnapshot = _backend.copyLatestSnapshot(snapshot, static_cast<std::size_t>(maxPoints));
            std::ostringstream out;
            out << std::fixed << std::setprecision(6)
                << "{\"ok\":true,\"cmd\":\"" << sim::protocol::cmd::GetSnapshot << "\""
                << ",\"has_snapshot\":" << boolToJson(hasSnapshot)
                << ",\"count\":" << snapshot.size()
                << ",\"particles\":[";
            for (std::size_t i = 0; i < snapshot.size(); ++i) {
                const RenderParticle &p = snapshot[i];
                if (i > 0) {
                    out << ",";
                }
                out << "[" << p.x << "," << p.y << "," << p.z << ","
                    << p.mass << "," << p.pressureNorm << "," << p.temperature << "]";
            }
            out << "]}";
            return out.str();
        }

        if (cmd == sim::protocol::cmd::Pause) {
            _backend.setPaused(true);
            return makeOkResponse(cmd);
        }
    if (cmd == sim::protocol::cmd::Resume) {
        _backend.setPaused(false);
        return makeOkResponse(cmd);
    }
    if (cmd == sim::protocol::cmd::Toggle) {
        _backend.togglePaused();
        return makeOkResponse(cmd);
    }
    if (cmd == sim::protocol::cmd::Reset) {
        _backend.requestReset();
        return makeOkResponse(cmd);
    }
    if (cmd == sim::protocol::cmd::Recover) {
        _backend.requestRecover();
        return makeOkResponse(cmd);
    }
    if (cmd == sim::protocol::cmd::Step) {
        int count = 1;
        extractJsonNumber(request, "count", count);
        if (count < 1) {
            count = 1;
        } else if (count > 100000) {
            count = 100000;
        }
        for (int i = 0; i < count; ++i) {
            _backend.stepOnce();
        }
        return makeOkResponse(cmd);
    }
    if (cmd == sim::protocol::cmd::SetDt) {
        double dt = 0.0;
        if (!extractJsonNumber(request, "value", dt) || dt <= 0.0) {
            return makeErrorResponse(cmd, "invalid dt value");
        }
        _backend.setDt(static_cast<float>(dt));
        return makeOkResponse(cmd);
    }
    if (cmd == sim::protocol::cmd::SetSolver) {
        std::string value;
        if (!extractJsonString(request, "value", value)) {
            return makeErrorResponse(cmd, "missing solver value");
        }
        _backend.setSolverMode(value);
        return makeOkResponse(cmd);
    }
    if (cmd == sim::protocol::cmd::SetIntegrator) {
        std::string value;
        if (!extractJsonString(request, "value", value)) {
            return makeErrorResponse(cmd, "missing integrator value");
        }
        _backend.setIntegratorMode(value);
        return makeOkResponse(cmd);
    }
    if (cmd == sim::protocol::cmd::SetParticleCount) {
        unsigned long long value = 0;
        if (!extractJsonNumber(request, "value", value) || value < 2ull) {
            return makeErrorResponse(cmd, "invalid particle count");
        }
        const unsigned long long clamped = (value > 100000000ull) ? 100000000ull : value;
        _backend.setParticleCount(static_cast<std::uint32_t>(clamped));
        return makeOkResponse(cmd);
    }
    if (cmd == sim::protocol::cmd::SetSph) {
        bool value = false;
        if (!extractJsonBool(request, "value", value)) {
            return makeErrorResponse(cmd, "missing bool sph value");
        }
        _backend.setSphEnabled(value);
        return makeOkResponse(cmd);
    }
    if (cmd == sim::protocol::cmd::SetOctree) {
        double theta = 0.0;
        double softening = 0.0;
        if (!extractJsonNumber(request, "theta", theta) || theta <= 0.0) {
            return makeErrorResponse(cmd, "invalid theta");
        }
        if (!extractJsonNumber(request, "softening", softening) || softening <= 0.0) {
            return makeErrorResponse(cmd, "invalid softening");
        }
        _backend.setOctreeParameters(static_cast<float>(theta), static_cast<float>(softening));
        return makeOkResponse(cmd);
    }
    if (cmd == sim::protocol::cmd::SetSphParams) {
        double h = 0.0;
        double restDensity = 0.0;
        double gasConstant = 0.0;
        double viscosity = 0.0;
        if (!extractJsonNumber(request, "h", h) || h <= 0.0
            || !extractJsonNumber(request, "rest_density", restDensity) || restDensity <= 0.0
            || !extractJsonNumber(request, "gas_constant", gasConstant) || gasConstant <= 0.0
            || !extractJsonNumber(request, "viscosity", viscosity) || viscosity < 0.0) {
            return makeErrorResponse(cmd, "invalid sph params");
        }
        _backend.setSphParameters(
            static_cast<float>(h),
            static_cast<float>(restDensity),
            static_cast<float>(gasConstant),
            static_cast<float>(viscosity)
        );
        return makeOkResponse(cmd);
    }
    if (cmd == sim::protocol::cmd::SetEnergyMeasure) {
        unsigned int everySteps = 0;
        unsigned int sampleLimit = 0;
        if (!extractJsonNumber(request, "every_steps", everySteps) || everySteps < 1u
            || !extractJsonNumber(request, "sample_limit", sampleLimit) || sampleLimit < 2u) {
            return makeErrorResponse(cmd, "invalid energy measure config");
        }
        _backend.setEnergyMeasurementConfig(everySteps, sampleLimit);
        return makeOkResponse(cmd);
    }
    if (cmd == sim::protocol::cmd::Load) {
        std::string path;
        if (!extractJsonString(request, "path", path) || path.empty()) {
            return makeErrorResponse(cmd, "missing path");
        }
        std::string format = "auto";
        extractJsonString(request, "format", format);
        _backend.setInitialStateFile(path, format);
        _backend.requestReset();
        return makeOkResponse(cmd);
    }
    if (cmd == sim::protocol::cmd::Export) {
        std::string path;
        std::string format;
        extractJsonString(request, "path", path);
        if (!extractJsonString(request, "format", format)) {
            format = "vtk";
        }
        _backend.requestExportSnapshot(path, format);
        return makeOkResponse(cmd);
    }
    if (cmd == sim::protocol::cmd::Shutdown) {
        _shutdownRequested.store(true);
        return makeOkResponse(cmd);
    }

        return makeErrorResponse(cmd, "unknown command");
    } catch (const std::exception &ex) {
        return makeErrorResponse("request", ex.what());
    } catch (...) {
        return makeErrorResponse("request", "unknown request exception");
    }
}
