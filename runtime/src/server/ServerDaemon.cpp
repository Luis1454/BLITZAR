// File: runtime/src/server/ServerDaemon.cpp
// Purpose: Runtime integration surface for BLITZAR clients and protocols.

#include "server/ServerDaemon.hpp"
#include "config/SimulationModes.hpp"
#include "platform/SocketPlatform.hpp"
#include "protocol/ServerJsonCodec.hpp"
#include "protocol/ServerProtocol.hpp"
#include "server/SimulationServer.hpp"
#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
/// Description: Executes the trim operation.
static std::string trim(const std::string& input)
{
    const auto begin = std::find_if_not(input.begin(), input.end(),
                                        [](unsigned char c) { return std::isspace(c) != 0; });
    const auto end = std::find_if_not(input.rbegin(), input.rend(), [](unsigned char c) {
                         return std::isspace(c) != 0;
                     }).base();
    if (begin >= end)
        return {};
    return std::string(begin, end);
}
/// Description: Executes the asBytes operation.
static grav_socket::ConstBytes asBytes(std::string_view text)
{
    return grav_socket::ConstBytes{reinterpret_cast<const std::byte*>(text.data()), text.size()};
}
/// Description: Executes the sendAll operation.
static bool sendAll(grav_socket::Handle socketHandle, grav_socket::ConstBytes bytes)
{
    std::size_t offset = 0;
    while (offset < bytes.size) {
        const int sent = grav_socket::sendBytes(socketHandle, bytes.subview(offset));
        if (sent <= 0)
            return false;
        offset += static_cast<std::size_t>(sent);
    }
    return true;
}
/// Description: Executes the serverDaemonError operation.
static std::string serverDaemonError(std::string_view operation, std::string_view detail)
{
    return std::string("[ipc] ") + std::string(operation) + ": " + std::string(detail);
}
/// Description: Executes the ServerDaemon operation.
ServerDaemon::ServerDaemon(SimulationServer& server, std::string authToken)
    : _server(server),
      _running(false),
      _shutdownRequested(false),
      _acceptThread(),
      _listenSocket(grav_socket::invalidHandle()),
      _bindAddress("127.0.0.1"),
      _authToken(std::move(authToken)),
      _port(0),
      _networkInitialized(false),
      _socketMutex(),
      /// Description: Executes the _clientThreads operation.
      _clientThreads()
{
}
/// Description: Releases resources owned by ServerDaemon.
ServerDaemon::~ServerDaemon()
{
    /// Description: Executes the stop operation.
    stop();
}
/// Description: Executes the start operation.
bool ServerDaemon::start(std::uint16_t port, const std::string& bindAddress)
{
    try {
        if (_running.load()) {
            return true;
        }
        if (!grav_socket::initializeSocketLayer()) {
            std::cerr << "[ipc] failed to initialize socket layer\n";
            return false;
        }
        _networkInitialized = true;
        const grav_socket::Handle listenSocket = grav_socket::createTcpSocket();
        if (!grav_socket::isValid(listenSocket)) {
            std::cerr << "[ipc] failed to create socket\n";
            /// Description: Executes the stop operation.
            stop();
            return false;
        }
        /// Description: Executes the setReuseAddress operation.
        grav_socket::setReuseAddress(listenSocket, true);
        if (!grav_socket::bindIpv4(listenSocket, bindAddress, port)) {
            std::cerr << "[ipc] bind failed on " << bindAddress << ":" << port << "\n";
            /// Description: Executes the closeSocket operation.
            grav_socket::closeSocket(listenSocket);
            /// Description: Executes the stop operation.
            stop();
            return false;
        }
        if (!grav_socket::listenSocket(listenSocket, 8)) {
            std::cerr << "[ipc] listen failed\n";
            /// Description: Executes the closeSocket operation.
            grav_socket::closeSocket(listenSocket);
            /// Description: Executes the stop operation.
            stop();
            return false;
        }
        {
            /// Description: Executes the lock operation.
            std::lock_guard<std::mutex> lock(_socketMutex);
            _listenSocket = listenSocket;
            _bindAddress = bindAddress;
            _port = port;
        }
        _shutdownRequested.store(false);
        _running.store(true);
        _acceptThread = std::thread(&ServerDaemon::acceptLoop, this);
        return true;
    }
    catch (const std::exception& ex) {
        std::cerr << serverDaemonError("start", ex.what()) << "\n";
        /// Description: Executes the stop operation.
        stop();
        return false;
    }
    catch (...) {
        std::cerr << serverDaemonError("start", "non-standard exception") << "\n";
        /// Description: Executes the stop operation.
        stop();
        return false;
    }
}
/// Description: Executes the stop operation.
void ServerDaemon::stop()
{
    try {
        _running.store(false);
        grav_socket::Handle listenSocket;
        {
            /// Description: Executes the lock operation.
            std::lock_guard<std::mutex> lock(_socketMutex);
            listenSocket = _listenSocket;
            _listenSocket = grav_socket::invalidHandle();
        }
        /// Description: Executes the closeSocket operation.
        grav_socket::closeSocket(listenSocket);
        if (_acceptThread.joinable()) {
            _acceptThread.join();
        }
        for (std::thread& thread : _clientThreads)
            if (thread.joinable()) {
                thread.join();
            }
        _clientThreads.clear();
        if (_networkInitialized) {
            /// Description: Executes the shutdownSocketLayer operation.
            grav_socket::shutdownSocketLayer();
            _networkInitialized = false;
        }
    }
    catch (const std::exception& ex) {
        std::cerr << serverDaemonError("stop", ex.what()) << "\n";
    }
    catch (...) {
        std::cerr << serverDaemonError("stop", "non-standard exception") << "\n";
    }
    _running.store(false);
    _listenSocket = grav_socket::invalidHandle();
    _clientThreads.clear();
    if (_networkInitialized) {
        try {
            /// Description: Executes the shutdownSocketLayer operation.
            grav_socket::shutdownSocketLayer();
        }
        catch (const std::exception& ex) {
            std::cerr << serverDaemonError("stop shutdownSocketLayer", ex.what()) << "\n";
        }
        catch (...) {
            /// Description: Executes the serverDaemonError operation.
            std::cerr << serverDaemonError("stop shutdownSocketLayer", "non-standard exception")
                      << "\n";
        }
        _networkInitialized = false;
    }
}
/// Description: Executes the isRunning operation.
bool ServerDaemon::isRunning() const
{
    return _running.load();
}
/// Description: Executes the shutdownRequested operation.
bool ServerDaemon::shutdownRequested() const
{
    return _shutdownRequested.load();
}
/// Description: Executes the acceptLoop operation.
void ServerDaemon::acceptLoop()
{
    try {
        while (_running.load()) {
            grav_socket::Handle listenSocket;
            {
                /// Description: Executes the lock operation.
                std::lock_guard<std::mutex> lock(_socketMutex);
                listenSocket = _listenSocket;
            }
            if (!grav_socket::isValid(listenSocket)) {
                break;
            }
            if (!grav_socket::waitReadable(listenSocket, 200)) {
                continue;
            }
            const grav_socket::Handle clientSocket = grav_socket::acceptSocket(listenSocket);
            if (!grav_socket::isValid(clientSocket)) {
                continue;
            }
            /// Description: Executes the setSocketTimeoutMs operation.
            grav_socket::setSocketTimeoutMs(clientSocket, 500);
            _clientThreads.emplace_back(&ServerDaemon::handleClient, this, clientSocket);
        }
    }
    catch (const std::exception& ex) {
        std::cerr << serverDaemonError("acceptLoop", ex.what()) << "\n";
        _running.store(false);
    }
    catch (...) {
        std::cerr << serverDaemonError("acceptLoop", "non-standard exception") << "\n";
        _running.store(false);
    }
}
/// Description: Executes the handleClient operation.
void ServerDaemon::handleClient(SocketHandle client)
{
    try {
        const grav_socket::Handle clientSocket = client;
        std::string buffer;
        buffer.reserve(2048);
        std::array<char, 2048> chunk{};
        while (_running.load()) {
            const int received = grav_socket::recvBytes(
                clientSocket, grav_socket::MutableBytes{reinterpret_cast<std::byte*>(chunk.data()),
                                                        chunk.size()});
            if (received <= 0) {
                if (received < 0 && grav_socket::wouldBlockOrTimeoutLastError() &&
                    _running.load()) {
                    continue;
                }
                break;
            }
            buffer.append(chunk.data(), static_cast<std::size_t>(received));
            std::size_t newline = buffer.find('\n');
            while (newline != std::string::npos) {
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
                    /// Description: Executes the closeSocket operation.
                    grav_socket::closeSocket(clientSocket);
                    return;
                }
                if (_shutdownRequested.load()) {
                    /// Description: Executes the closeSocket operation.
                    grav_socket::closeSocket(clientSocket);
                    return;
                }
                newline = buffer.find('\n');
            }
        }
        /// Description: Executes the closeSocket operation.
        grav_socket::closeSocket(clientSocket);
    }
    catch (const std::exception& ex) {
        std::cerr << serverDaemonError("handleClient", ex.what()) << "\n";
        /// Description: Executes the closeSocket operation.
        grav_socket::closeSocket(static_cast<grav_socket::Handle>(client));
    }
    catch (...) {
        std::cerr << serverDaemonError("handleClient", "non-standard exception") << "\n";
        /// Description: Executes the closeSocket operation.
        grav_socket::closeSocket(static_cast<grav_socket::Handle>(client));
    }
}
/// Description: Executes the processRequest operation.
std::string ServerDaemon::processRequest(const std::string& request)
{
    try {
        grav_protocol::ServerCommandRequest envelope{};
        std::string parseError;
        if (!grav_protocol::ServerJsonCodec::parseCommandRequest(request, envelope, parseError)) {
            return grav_protocol::ServerJsonCodec::makeErrorResponse("unknown", parseError);
        }
        if (!_authToken.empty() && envelope.token != _authToken) {
            return grav_protocol::ServerJsonCodec::makeErrorResponse("auth", "unauthorized");
        }
        const std::string& cmd = envelope.cmd;
        if (cmd == grav_protocol::Status)
            return grav_protocol::ServerJsonCodec::makeStatusResponse(_server.getStats());
        if (cmd == grav_protocol::GetSnapshot) {
            std::uint32_t maxPoints = grav_protocol::kSnapshotDefaultPoints;
            /// Description: Executes the readNumber operation.
            grav_protocol::ServerJsonCodec::readNumber(request, "max_points", maxPoints);
            maxPoints = grav_protocol::clampSnapshotPoints(maxPoints);
            std::vector<RenderParticle> snapshot;
            std::size_t sourceSize = 0u;
            const bool hasSnapshot = _server.copyLatestSnapshot(
                snapshot, static_cast<std::size_t>(maxPoints), &sourceSize);
            return grav_protocol::ServerJsonCodec::makeSnapshotResponse(hasSnapshot, snapshot,
                                                                        sourceSize);
        }
        if (cmd == grav_protocol::Pause) {
            _server.setPaused(true);
            return grav_protocol::ServerJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == grav_protocol::Resume) {
            _server.setPaused(false);
            return grav_protocol::ServerJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == grav_protocol::Toggle) {
            _server.togglePaused();
            return grav_protocol::ServerJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == grav_protocol::Reset) {
            _server.requestReset();
            return grav_protocol::ServerJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == grav_protocol::Recover) {
            _server.requestRecover();
            return grav_protocol::ServerJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == grav_protocol::Step) {
            int count = 1;
            /// Description: Executes the readNumber operation.
            grav_protocol::ServerJsonCodec::readNumber(request, "count", count);
            if (count < 1)
                count = 1;
            else if (count > 100000)
                count = 100000;
            for (int i = 0; i < count; ++i)
                _server.stepOnce();
            return grav_protocol::ServerJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == grav_protocol::SetDt) {
            double dt = 0.0;
            if (!grav_protocol::ServerJsonCodec::readNumber(request, "value", dt) || dt <= 0.0) {
                return grav_protocol::ServerJsonCodec::makeErrorResponse(cmd, "invalid dt value");
            }
            _server.setDt(static_cast<float>(dt));
            return grav_protocol::ServerJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == grav_protocol::SetSolver) {
            std::string value;
            if (!grav_protocol::ServerJsonCodec::readString(request, "value", value)) {
                return grav_protocol::ServerJsonCodec::makeErrorResponse(cmd,
                                                                         "missing solver value");
            }
            std::string canonical;
            if (!grav_modes::normalizeSolver(value, canonical)) {
                return grav_protocol::ServerJsonCodec::makeErrorResponse(cmd,
                                                                         "invalid solver value");
            }
            if (!grav_modes::isSupportedSolverIntegratorPair(canonical,
                                                             _server.getStats().integratorName)) {
                return grav_protocol::ServerJsonCodec::makeErrorResponse(
                    cmd, "unsupported solver/integrator combination: octree_gpu requires euler");
            }
            _server.setSolverMode(canonical);
            return grav_protocol::ServerJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == grav_protocol::SetIntegrator) {
            std::string value;
            if (!grav_protocol::ServerJsonCodec::readString(request, "value", value)) {
                return grav_protocol::ServerJsonCodec::makeErrorResponse(
                    cmd, "missing integrator value");
            }
            std::string canonical;
            if (!grav_modes::normalizeIntegrator(value, canonical)) {
                return grav_protocol::ServerJsonCodec::makeErrorResponse(
                    cmd, "invalid integrator value");
            }
            if (!grav_modes::isSupportedSolverIntegratorPair(_server.getStats().solverName,
                                                             canonical)) {
                return grav_protocol::ServerJsonCodec::makeErrorResponse(
                    cmd, "unsupported solver/integrator combination: octree_gpu requires euler");
            }
            _server.setIntegratorMode(canonical);
            return grav_protocol::ServerJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == grav_protocol::SetPerformanceProfile) {
            std::string value;
            if (!grav_protocol::ServerJsonCodec::readString(request, "value", value)) {
                return grav_protocol::ServerJsonCodec::makeErrorResponse(
                    cmd, "missing performance profile");
            }
            _server.setPerformanceProfile(value);
            return grav_protocol::ServerJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == grav_protocol::SetParticleCount) {
            std::uint64_t value = 0;
            if (!grav_protocol::ServerJsonCodec::readNumber(request, "value", value) ||
                value < 2ull) {
                return grav_protocol::ServerJsonCodec::makeErrorResponse(cmd,
                                                                         "invalid particle count");
            }
            const std::uint64_t clamped = (value > 100000000ull) ? 100000000ull : value;
            _server.setParticleCount(static_cast<std::uint32_t>(clamped));
            return grav_protocol::ServerJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == grav_protocol::SetSph) {
            bool value = false;
            if (!grav_protocol::ServerJsonCodec::readBool(request, "value", value)) {
                return grav_protocol::ServerJsonCodec::makeErrorResponse(cmd,
                                                                         "missing bool sph value");
            }
            _server.setSphEnabled(value);
            return grav_protocol::ServerJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == grav_protocol::SetOctree) {
            double theta = 0.0;
            double softening = 0.0;
            if (!grav_protocol::ServerJsonCodec::readNumber(request, "theta", theta) ||
                theta <= 0.0) {
                return grav_protocol::ServerJsonCodec::makeErrorResponse(cmd, "invalid theta");
            }
            if (!grav_protocol::ServerJsonCodec::readNumber(request, "softening", softening) ||
                softening <= 0.0) {
                return grav_protocol::ServerJsonCodec::makeErrorResponse(cmd, "invalid softening");
            }
            _server.setOctreeParameters(static_cast<float>(theta), static_cast<float>(softening));
            return grav_protocol::ServerJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == grav_protocol::SetSphParams) {
            double h = 0.0;
            double restDensity = 0.0;
            double gasConstant = 0.0;
            double viscosity = 0.0;
            if (!grav_protocol::ServerJsonCodec::readNumber(request, "h", h) || h <= 0.0 ||
                !grav_protocol::ServerJsonCodec::readNumber(request, "rest_density", restDensity) ||
                restDensity <= 0.0 ||
                !grav_protocol::ServerJsonCodec::readNumber(request, "gas_constant", gasConstant) ||
                gasConstant <= 0.0 ||
                !grav_protocol::ServerJsonCodec::readNumber(request, "viscosity", viscosity) ||
                viscosity < 0.0) {
                return grav_protocol::ServerJsonCodec::makeErrorResponse(cmd, "invalid sph params");
            }
            _server.setSphParameters(static_cast<float>(h), static_cast<float>(restDensity),
                                     static_cast<float>(gasConstant),
                                     static_cast<float>(viscosity));
            return grav_protocol::ServerJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == grav_protocol::SetSubsteps) {
            double targetDt = 0.0;
            std::uint32_t maxSubsteps = 0;
            if (!grav_protocol::ServerJsonCodec::readNumber(request, "target_dt", targetDt) ||
                targetDt < 0.0 ||
                !grav_protocol::ServerJsonCodec::readNumber(request, "max_substeps", maxSubsteps) ||
                maxSubsteps < 1u) {
                return grav_protocol::ServerJsonCodec::makeErrorResponse(cmd,
                                                                         "invalid substep policy");
            }
            _server.setSubstepPolicy(static_cast<float>(targetDt), maxSubsteps);
            return grav_protocol::ServerJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == grav_protocol::SetEnergyMeasure) {
            std::uint32_t everySteps = 0;
            std::uint32_t sampleLimit = 0;
            if (!grav_protocol::ServerJsonCodec::readNumber(request, "every_steps", everySteps) ||
                everySteps < 1u ||
                !grav_protocol::ServerJsonCodec::readNumber(request, "sample_limit", sampleLimit) ||
                sampleLimit < 2u) {
                return grav_protocol::ServerJsonCodec::makeErrorResponse(
                    cmd, "invalid energy measure config");
            }
            _server.setEnergyMeasurementConfig(everySteps, sampleLimit);
            return grav_protocol::ServerJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == grav_protocol::SetGpuTelemetry) {
            bool enabled = false;
            if (!grav_protocol::ServerJsonCodec::readBool(request, "value", enabled)) {
                return grav_protocol::ServerJsonCodec::makeErrorResponse(
                    cmd, "missing bool gpu telemetry value");
            }
            _server.setGpuTelemetryEnabled(enabled);
            return grav_protocol::ServerJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == grav_protocol::SetSnapshotPublishCadence) {
            std::uint32_t periodMs = 0;
            if (!grav_protocol::ServerJsonCodec::readNumber(request, "period_ms", periodMs) ||
                periodMs < 1u) {
                return grav_protocol::ServerJsonCodec::makeErrorResponse(
                    cmd, "invalid snapshot cadence");
            }
            _server.setSnapshotPublishPeriodMs(periodMs);
            return grav_protocol::ServerJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == grav_protocol::SetSnapshotTransferCap) {
            std::uint32_t maxPoints = 0;
            if (!grav_protocol::ServerJsonCodec::readNumber(request, "max_points", maxPoints) ||
                maxPoints < 1u) {
                return grav_protocol::ServerJsonCodec::makeErrorResponse(
                    cmd, "invalid snapshot transfer cap");
            }
            _server.setSnapshotTransferCap(maxPoints);
            return grav_protocol::ServerJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == grav_protocol::Load) {
            std::string path;
            if (!grav_protocol::ServerJsonCodec::readString(request, "path", path) ||
                path.empty()) {
                return grav_protocol::ServerJsonCodec::makeErrorResponse(cmd, "missing path");
            }
            std::string format = "auto";
            /// Description: Executes the readString operation.
            grav_protocol::ServerJsonCodec::readString(request, "format", format);
            _server.setInitialStateFile(path, format);
            _server.requestReset();
            return grav_protocol::ServerJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == grav_protocol::Export) {
            std::string path;
            std::string format;
            /// Description: Executes the readString operation.
            grav_protocol::ServerJsonCodec::readString(request, "path", path);
            if (!grav_protocol::ServerJsonCodec::readString(request, "format", format)) {
                format = "vtk";
            }
            _server.requestExportSnapshot(path, format);
            return grav_protocol::ServerJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == grav_protocol::SaveCheckpoint) {
            std::string path;
            if (!grav_protocol::ServerJsonCodec::readString(request, "path", path) ||
                path.empty()) {
                return grav_protocol::ServerJsonCodec::makeErrorResponse(cmd, "missing path");
            }
            if (!_server.saveCheckpoint(path)) {
                return grav_protocol::ServerJsonCodec::makeErrorResponse(
                    cmd, "failed to save checkpoint");
            }
            return grav_protocol::ServerJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == grav_protocol::LoadCheckpoint) {
            std::string path;
            if (!grav_protocol::ServerJsonCodec::readString(request, "path", path) ||
                path.empty()) {
                return grav_protocol::ServerJsonCodec::makeErrorResponse(cmd, "missing path");
            }
            std::string error;
            if (!_server.loadCheckpoint(path, &error)) {
                return grav_protocol::ServerJsonCodec::makeErrorResponse(
                    cmd, error.empty() ? "failed to load checkpoint" : error);
            }
            return grav_protocol::ServerJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == grav_protocol::Shutdown) {
            _shutdownRequested.store(true);
            return grav_protocol::ServerJsonCodec::makeOkResponse(cmd);
        }
        return grav_protocol::ServerJsonCodec::makeErrorResponse(cmd, "unknown command");
    }
    catch (const std::exception& ex) {
        return grav_protocol::ServerJsonCodec::makeErrorResponse(
            "request", serverDaemonError("processRequest", ex.what()));
    }
    catch (...) {
        return grav_protocol::ServerJsonCodec::makeErrorResponse(
            "request", serverDaemonError("processRequest", "non-standard exception"));
    }
}
