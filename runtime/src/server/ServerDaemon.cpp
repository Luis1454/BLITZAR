/*
 * @file runtime/src/server/ServerDaemon.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Runtime implementation for protocol, command, client, and FFI boundaries.
 */

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

/*
 * @brief Documents the trim operation contract.
 * @param input Input value used by this contract.
 * @return std::string value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
static std::string trim(const std::string& input)
{
    const auto begin = std::find_if_not(input.begin(), input.end(), [](unsigned char c) {
        return std::isspace(c) != 0;
    });
    const auto end = std::find_if_not(input.rbegin(), input.rend(), [](unsigned char c) {
                         return std::isspace(c) != 0;
                     }).base();
    if (begin >= end)
        return {};
    return std::string(begin, end);
}

/*
 * @brief Documents the as bytes operation contract.
 * @param text Input value used by this contract.
 * @return bltzr_socket::ConstBytes value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
static bltzr_socket::ConstBytes asBytes(std::string_view text)
{
    return bltzr_socket::ConstBytes{reinterpret_cast<const std::byte*>(text.data()), text.size()};
}

/*
 * @brief Documents the send all operation contract.
 * @param socketHandle Input value used by this contract.
 * @param bytes Input value used by this contract.
 * @return bool value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
static bool sendAll(bltzr_socket::Handle socketHandle, bltzr_socket::ConstBytes bytes)
{
    std::size_t offset = 0;
    while (offset < bytes.size) {
        const int sent = bltzr_socket::sendBytes(socketHandle, bytes.subview(offset));
        if (sent <= 0)
            return false;
        offset += static_cast<std::size_t>(sent);
    }
    return true;
}

/*
 * @brief Documents the server daemon error operation contract.
 * @param operation Input value used by this contract.
 * @param detail Input value used by this contract.
 * @return std::string value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
static std::string serverDaemonError(std::string_view operation, std::string_view detail)
{
    return std::string("[ipc] ") + std::string(operation) + ": " + std::string(detail);
}

/*
 * @brief Documents the server daemon operation contract.
 * @param server Input value used by this contract.
 * @param authToken Input value used by this contract.
 * @return ServerDaemon:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
ServerDaemon::ServerDaemon(SimulationServer& server, std::string authToken)
    /*
     * @brief Documents the server operation contract.
     * @param server Input value used by this contract.
     * @param false Input value used by this contract.
     * @param false Input value used by this contract.
     * @param _acceptThread Input value used by this contract.
     * @param invalidHandle Input value used by this contract.
     * @param _bindAddress Input value used by this contract.
     * @param authToken Input value used by this contract.
     * @param _port Input value used by this contract.
     * @param false Input value used by this contract.
     * @param _socketMutex Input value used by this contract.
     * @param _clientThreads Input value used by this contract.
     * @return : value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    : _server(server),
      _running(false),
      _shutdownRequested(false),
      _acceptThread(),
      _listenSocket(bltzr_socket::invalidHandle()),
      _bindAddress("127.0.0.1"),
      _authToken(std::move(authToken)),
      _port(0),
      _networkInitialized(false),
      _socketMutex(),
      _clientThreads()
{
}

/*
 * @brief Documents the ~server daemon operation contract.
 * @param None This contract does not take explicit parameters.
 * @return ServerDaemon:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
ServerDaemon::~ServerDaemon()
{
    stop();
}

/*
 * @brief Documents the start operation contract.
 * @param port Input value used by this contract.
 * @param bindAddress Input value used by this contract.
 * @return bool ServerDaemon:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool ServerDaemon::start(std::uint16_t port, const std::string& bindAddress)
{
    try {
        if (_running.load()) {
            return true;
        }
        if (!bltzr_socket::initializeSocketLayer()) {
            std::cerr << "[ipc] failed to initialize socket layer\n";
            return false;
        }
        _networkInitialized = true;
        const bltzr_socket::Handle listenSocket = bltzr_socket::createTcpSocket();
        if (!bltzr_socket::isValid(listenSocket)) {
            std::cerr << "[ipc] failed to create socket\n";
            stop();
            return false;
        }
        bltzr_socket::setReuseAddress(listenSocket, true);
        if (!bltzr_socket::bindIpv4(listenSocket, bindAddress, port)) {
            std::cerr << "[ipc] bind failed on " << bindAddress << ":" << port << "\n";
            bltzr_socket::closeSocket(listenSocket);
            stop();
            return false;
        }
        if (!bltzr_socket::listenSocket(listenSocket, 8)) {
            std::cerr << "[ipc] listen failed\n";
            bltzr_socket::closeSocket(listenSocket);
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
        _acceptThread = std::thread(&ServerDaemon::acceptLoop, this);
        return true;
    }
    catch (const std::exception& ex) {
        std::cerr << serverDaemonError("start", ex.what()) << "\n";
        stop();
        return false;
    }
    catch (...) {
        std::cerr << serverDaemonError("start", "non-standard exception") << "\n";
        stop();
        return false;
    }
}

/*
 * @brief Documents the stop operation contract.
 * @param None This contract does not take explicit parameters.
 * @return void ServerDaemon:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void ServerDaemon::stop()
{
    try {
        _running.store(false);
        bltzr_socket::Handle listenSocket;
        {
            std::lock_guard<std::mutex> lock(_socketMutex);
            listenSocket = _listenSocket;
            _listenSocket = bltzr_socket::invalidHandle();
        }
        bltzr_socket::closeSocket(listenSocket);
        if (_acceptThread.joinable()) {
            _acceptThread.join();
        }
        for (std::thread& thread : _clientThreads)
            if (thread.joinable()) {
                thread.join();
            }
        _clientThreads.clear();
        if (_networkInitialized) {
            bltzr_socket::shutdownSocketLayer();
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
    _listenSocket = bltzr_socket::invalidHandle();
    _clientThreads.clear();
    if (_networkInitialized) {
        try {
            bltzr_socket::shutdownSocketLayer();
        }
        catch (const std::exception& ex) {
            std::cerr << serverDaemonError("stop shutdownSocketLayer", ex.what()) << "\n";
        }
        catch (...) {
            std::cerr << serverDaemonError("stop shutdownSocketLayer", "non-standard exception")
                      << "\n";
        }
        _networkInitialized = false;
    }
}

/*
 * @brief Documents the is running operation contract.
 * @param None This contract does not take explicit parameters.
 * @return bool ServerDaemon:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool ServerDaemon::isRunning() const
{
    return _running.load();
}

/*
 * @brief Documents the shutdown requested operation contract.
 * @param None This contract does not take explicit parameters.
 * @return bool ServerDaemon:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool ServerDaemon::shutdownRequested() const
{
    return _shutdownRequested.load();
}

/*
 * @brief Documents the accept loop operation contract.
 * @param None This contract does not take explicit parameters.
 * @return void ServerDaemon:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void ServerDaemon::acceptLoop()
{
    try {
        while (_running.load()) {
            bltzr_socket::Handle listenSocket;
            {
                std::lock_guard<std::mutex> lock(_socketMutex);
                listenSocket = _listenSocket;
            }
            if (!bltzr_socket::isValid(listenSocket)) {
                break;
            }
            if (!bltzr_socket::waitReadable(listenSocket, 200)) {
                continue;
            }
            const bltzr_socket::Handle clientSocket = bltzr_socket::acceptSocket(listenSocket);
            if (!bltzr_socket::isValid(clientSocket)) {
                continue;
            }
            bltzr_socket::setSocketTimeoutMs(clientSocket, 500);
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

/*
 * @brief Documents the handle client operation contract.
 * @param client Input value used by this contract.
 * @return void ServerDaemon:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void ServerDaemon::handleClient(SocketHandle client)
{
    try {
        const bltzr_socket::Handle clientSocket = client;
        std::string buffer;
        buffer.reserve(2048);
        std::array<char, 2048> chunk{};
        while (_running.load()) {
            const int received = bltzr_socket::recvBytes(
                clientSocket, bltzr_socket::MutableBytes{reinterpret_cast<std::byte*>(chunk.data()),
                                                         chunk.size()});
            if (received <= 0) {
                if (received < 0 && bltzr_socket::wouldBlockOrTimeoutLastError() &&
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
                    bltzr_socket::closeSocket(clientSocket);
                    return;
                }
                if (_shutdownRequested.load()) {
                    bltzr_socket::closeSocket(clientSocket);
                    return;
                }
                newline = buffer.find('\n');
            }
        }
        bltzr_socket::closeSocket(clientSocket);
    }
    catch (const std::exception& ex) {
        std::cerr << serverDaemonError("handleClient", ex.what()) << "\n";
        bltzr_socket::closeSocket(static_cast<bltzr_socket::Handle>(client));
    }
    catch (...) {
        std::cerr << serverDaemonError("handleClient", "non-standard exception") << "\n";
        bltzr_socket::closeSocket(static_cast<bltzr_socket::Handle>(client));
    }
}

/*
 * @brief Documents the process request operation contract.
 * @param request Input value used by this contract.
 * @return std::string ServerDaemon:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
std::string ServerDaemon::processRequest(const std::string& request)
{
    try {
        bltzr_protocol::ServerCommandRequest envelope{};
        std::string parseError;
        if (!bltzr_protocol::ServerJsonCodec::parseCommandRequest(request, envelope, parseError)) {
            return bltzr_protocol::ServerJsonCodec::makeErrorResponse("unknown", parseError);
        }
        if (!_authToken.empty() && envelope.token != _authToken) {
            return bltzr_protocol::ServerJsonCodec::makeErrorResponse("auth", "unauthorized");
        }
        const std::string& cmd = envelope.cmd;
        if (cmd == bltzr_protocol::Status)
            return bltzr_protocol::ServerJsonCodec::makeStatusResponse(_server.getStats());
        if (cmd == bltzr_protocol::GetSnapshot) {
            std::uint32_t maxPoints = bltzr_protocol::kSnapshotDefaultPoints;
            bltzr_protocol::ServerJsonCodec::readNumber(request, "max_points", maxPoints);
            maxPoints = bltzr_protocol::clampSnapshotPoints(maxPoints);
            std::vector<RenderParticle> snapshot;
            std::size_t sourceSize = 0u;
            const bool hasSnapshot = _server.copyLatestSnapshot(
                snapshot, static_cast<std::size_t>(maxPoints), &sourceSize);
            return bltzr_protocol::ServerJsonCodec::makeSnapshotResponse(hasSnapshot, snapshot,
                                                                         sourceSize);
        }
        if (cmd == bltzr_protocol::Pause) {
            _server.setPaused(true);
            return bltzr_protocol::ServerJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == bltzr_protocol::Resume) {
            _server.setPaused(false);
            return bltzr_protocol::ServerJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == bltzr_protocol::Toggle) {
            _server.togglePaused();
            return bltzr_protocol::ServerJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == bltzr_protocol::Reset) {
            _server.requestReset();
            return bltzr_protocol::ServerJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == bltzr_protocol::Recover) {
            _server.requestRecover();
            return bltzr_protocol::ServerJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == bltzr_protocol::Step) {
            int count = 1;
            bltzr_protocol::ServerJsonCodec::readNumber(request, "count", count);
            if (count < 1)
                count = 1;
            else if (count > 100000)
                count = 100000;
            for (int i = 0; i < count; ++i)
                _server.stepOnce();
            return bltzr_protocol::ServerJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == bltzr_protocol::SetDt) {
            double dt = 0.0;
            if (!bltzr_protocol::ServerJsonCodec::readNumber(request, "value", dt) || dt <= 0.0) {
                return bltzr_protocol::ServerJsonCodec::makeErrorResponse(cmd, "invalid dt value");
            }
            _server.setDt(static_cast<float>(dt));
            return bltzr_protocol::ServerJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == bltzr_protocol::SetSolver) {
            std::string value;
            if (!bltzr_protocol::ServerJsonCodec::readString(request, "value", value)) {
                return bltzr_protocol::ServerJsonCodec::makeErrorResponse(cmd,
                                                                          "missing solver value");
            }
            std::string canonical;
            if (!bltzr_modes::normalizeSolver(value, canonical)) {
                return bltzr_protocol::ServerJsonCodec::makeErrorResponse(cmd,
                                                                          "invalid solver value");
            }
            if (!bltzr_modes::isSupportedSolverIntegratorPair(canonical,
                                                              _server.getStats().integratorName)) {
                return bltzr_protocol::ServerJsonCodec::makeErrorResponse(
                    cmd, "unsupported solver/integrator combination: octree_gpu requires euler");
            }
            _server.setSolverMode(canonical);
            return bltzr_protocol::ServerJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == bltzr_protocol::SetIntegrator) {
            std::string value;
            if (!bltzr_protocol::ServerJsonCodec::readString(request, "value", value)) {
                return bltzr_protocol::ServerJsonCodec::makeErrorResponse(
                    cmd, "missing integrator value");
            }
            std::string canonical;
            if (!bltzr_modes::normalizeIntegrator(value, canonical)) {
                return bltzr_protocol::ServerJsonCodec::makeErrorResponse(
                    cmd, "invalid integrator value");
            }
            if (!bltzr_modes::isSupportedSolverIntegratorPair(_server.getStats().solverName,
                                                              canonical)) {
                return bltzr_protocol::ServerJsonCodec::makeErrorResponse(
                    cmd, "unsupported solver/integrator combination: octree_gpu requires euler");
            }
            _server.setIntegratorMode(canonical);
            return bltzr_protocol::ServerJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == bltzr_protocol::SetPerformanceProfile) {
            std::string value;
            if (!bltzr_protocol::ServerJsonCodec::readString(request, "value", value)) {
                return bltzr_protocol::ServerJsonCodec::makeErrorResponse(
                    cmd, "missing performance profile");
            }
            _server.setPerformanceProfile(value);
            return bltzr_protocol::ServerJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == bltzr_protocol::SetParticleCount) {
            std::uint64_t value = 0;
            if (!bltzr_protocol::ServerJsonCodec::readNumber(request, "value", value) ||
                value < 2ull) {
                return bltzr_protocol::ServerJsonCodec::makeErrorResponse(cmd,
                                                                          "invalid particle count");
            }
            const std::uint64_t clamped = (value > 100000000ull) ? 100000000ull : value;
            _server.setParticleCount(static_cast<std::uint32_t>(clamped));
            return bltzr_protocol::ServerJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == bltzr_protocol::SetSph) {
            bool value = false;
            if (!bltzr_protocol::ServerJsonCodec::readBool(request, "value", value)) {
                return bltzr_protocol::ServerJsonCodec::makeErrorResponse(cmd,
                                                                          "missing bool sph value");
            }
            _server.setSphEnabled(value);
            return bltzr_protocol::ServerJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == bltzr_protocol::SetOctree) {
            double theta = 0.0;
            double softening = 0.0;
            if (!bltzr_protocol::ServerJsonCodec::readNumber(request, "theta", theta) ||
                theta <= 0.0) {
                return bltzr_protocol::ServerJsonCodec::makeErrorResponse(cmd, "invalid theta");
            }
            if (!bltzr_protocol::ServerJsonCodec::readNumber(request, "softening", softening) ||
                softening <= 0.0) {
                return bltzr_protocol::ServerJsonCodec::makeErrorResponse(cmd, "invalid softening");
            }
            _server.setOctreeParameters(static_cast<float>(theta), static_cast<float>(softening));
            return bltzr_protocol::ServerJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == bltzr_protocol::SetSphParams) {
            double h = 0.0;
            double restDensity = 0.0;
            double gasConstant = 0.0;
            double viscosity = 0.0;
            if (!bltzr_protocol::ServerJsonCodec::readNumber(request, "h", h) || h <= 0.0 ||
                !bltzr_protocol::ServerJsonCodec::readNumber(request, "rest_density",
                                                             restDensity) ||
                restDensity <= 0.0 ||
                !bltzr_protocol::ServerJsonCodec::readNumber(request, "gas_constant",
                                                             gasConstant) ||
                gasConstant <= 0.0 ||
                !bltzr_protocol::ServerJsonCodec::readNumber(request, "viscosity", viscosity) ||
                viscosity < 0.0) {
                return bltzr_protocol::ServerJsonCodec::makeErrorResponse(cmd,
                                                                          "invalid sph params");
            }
            _server.setSphParameters(static_cast<float>(h), static_cast<float>(restDensity),
                                     static_cast<float>(gasConstant),
                                     static_cast<float>(viscosity));
            return bltzr_protocol::ServerJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == bltzr_protocol::SetSubsteps) {
            double targetDt = 0.0;
            std::uint32_t maxSubsteps = 0;
            if (!bltzr_protocol::ServerJsonCodec::readNumber(request, "target_dt", targetDt) ||
                targetDt < 0.0 ||
                !bltzr_protocol::ServerJsonCodec::readNumber(request, "max_substeps",
                                                             maxSubsteps) ||
                maxSubsteps < 1u) {
                return bltzr_protocol::ServerJsonCodec::makeErrorResponse(cmd,
                                                                          "invalid substep policy");
            }
            _server.setSubstepPolicy(static_cast<float>(targetDt), maxSubsteps);
            return bltzr_protocol::ServerJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == bltzr_protocol::SetEnergyMeasure) {
            std::uint32_t everySteps = 0;
            std::uint32_t sampleLimit = 0;
            if (!bltzr_protocol::ServerJsonCodec::readNumber(request, "every_steps", everySteps) ||
                everySteps < 1u ||
                !bltzr_protocol::ServerJsonCodec::readNumber(request, "sample_limit",
                                                             sampleLimit) ||
                sampleLimit < 2u) {
                return bltzr_protocol::ServerJsonCodec::makeErrorResponse(
                    cmd, "invalid energy measure config");
            }
            _server.setEnergyMeasurementConfig(everySteps, sampleLimit);
            return bltzr_protocol::ServerJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == bltzr_protocol::SetGpuTelemetry) {
            bool enabled = false;
            if (!bltzr_protocol::ServerJsonCodec::readBool(request, "value", enabled)) {
                return bltzr_protocol::ServerJsonCodec::makeErrorResponse(
                    cmd, "missing bool gpu telemetry value");
            }
            _server.setGpuTelemetryEnabled(enabled);
            return bltzr_protocol::ServerJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == bltzr_protocol::SetSnapshotPublishCadence) {
            std::uint32_t periodMs = 0;
            if (!bltzr_protocol::ServerJsonCodec::readNumber(request, "period_ms", periodMs) ||
                periodMs < 1u) {
                return bltzr_protocol::ServerJsonCodec::makeErrorResponse(
                    cmd, "invalid snapshot cadence");
            }
            _server.setSnapshotPublishPeriodMs(periodMs);
            return bltzr_protocol::ServerJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == bltzr_protocol::SetSnapshotTransferCap) {
            std::uint32_t maxPoints = 0;
            if (!bltzr_protocol::ServerJsonCodec::readNumber(request, "max_points", maxPoints) ||
                maxPoints < 1u) {
                return bltzr_protocol::ServerJsonCodec::makeErrorResponse(
                    cmd, "invalid snapshot transfer cap");
            }
            _server.setSnapshotTransferCap(maxPoints);
            return bltzr_protocol::ServerJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == bltzr_protocol::Load) {
            std::string path;
            if (!bltzr_protocol::ServerJsonCodec::readString(request, "path", path) ||
                path.empty()) {
                return bltzr_protocol::ServerJsonCodec::makeErrorResponse(cmd, "missing path");
            }
            std::string format = "auto";
            bltzr_protocol::ServerJsonCodec::readString(request, "format", format);
            _server.setInitialStateFile(path, format);
            _server.requestReset();
            return bltzr_protocol::ServerJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == bltzr_protocol::Export) {
            std::string path;
            std::string format;
            bltzr_protocol::ServerJsonCodec::readString(request, "path", path);
            if (!bltzr_protocol::ServerJsonCodec::readString(request, "format", format)) {
                format = "vtk";
            }
            _server.requestExportSnapshot(path, format);
            return bltzr_protocol::ServerJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == bltzr_protocol::SaveCheckpoint) {
            std::string path;
            if (!bltzr_protocol::ServerJsonCodec::readString(request, "path", path) ||
                path.empty()) {
                return bltzr_protocol::ServerJsonCodec::makeErrorResponse(cmd, "missing path");
            }
            if (!_server.saveCheckpoint(path)) {
                return bltzr_protocol::ServerJsonCodec::makeErrorResponse(
                    cmd, "failed to save checkpoint");
            }
            return bltzr_protocol::ServerJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == bltzr_protocol::LoadCheckpoint) {
            std::string path;
            if (!bltzr_protocol::ServerJsonCodec::readString(request, "path", path) ||
                path.empty()) {
                return bltzr_protocol::ServerJsonCodec::makeErrorResponse(cmd, "missing path");
            }
            std::string error;
            if (!_server.loadCheckpoint(path, &error)) {
                return bltzr_protocol::ServerJsonCodec::makeErrorResponse(
                    cmd, error.empty() ? "failed to load checkpoint" : error);
            }
            return bltzr_protocol::ServerJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == bltzr_protocol::Shutdown) {
            _shutdownRequested.store(true);
            return bltzr_protocol::ServerJsonCodec::makeOkResponse(cmd);
        }
        return bltzr_protocol::ServerJsonCodec::makeErrorResponse(cmd, "unknown command");
    }
    catch (const std::exception& ex) {
        return bltzr_protocol::ServerJsonCodec::makeErrorResponse(
            "request", serverDaemonError("processRequest", ex.what()));
    }
    catch (...) {
        return bltzr_protocol::ServerJsonCodec::makeErrorResponse(
            "request", serverDaemonError("processRequest", "non-standard exception"));
    }
}
