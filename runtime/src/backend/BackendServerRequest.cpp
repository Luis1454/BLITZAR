#include "backend/BackendServerRequest.hpp"
#include "config/SimulationModes.hpp"
#include "protocol/BackendJsonCodec.hpp"
#include "protocol/BackendProtocol.hpp"

#include <cstdint>
#include <exception>
#include <string>
#include <string_view>
#include <vector>

static std::string backendServerError(std::string_view operation, std::string_view detail)
{
    return std::string("[ipc] ") + std::string(operation) + ": " + std::string(detail);
}

std::string BackendServer::processRequest(const std::string &request)
{
    try {
        grav_protocol::BackendCommandRequest envelope{};
        std::string parseError;
        if (!grav_protocol::BackendJsonCodec::parseCommandRequest(request, envelope, parseError)) {
            return grav_protocol::BackendJsonCodec::makeErrorResponse("unknown", parseError);
        }
        if (!_authToken.empty() && envelope.token != _authToken) {
            return grav_protocol::BackendJsonCodec::makeErrorResponse("auth", "unauthorized");
        }

        const std::string &cmd = envelope.cmd;
        if (cmd == grav_protocol::Status) {
            return grav_protocol::BackendJsonCodec::makeStatusResponse(_backend.getStats());
        }
        if (cmd == grav_protocol::GetSnapshot) {
            std::uint32_t maxPoints = grav_protocol::kSnapshotDefaultPoints;
            grav_protocol::BackendJsonCodec::readNumber(request, "max_points", maxPoints);
            maxPoints = grav_protocol::clampSnapshotPoints(maxPoints);

            std::vector<RenderParticle> snapshot;
            const bool hasSnapshot = _backend.copyLatestSnapshot(snapshot, static_cast<std::size_t>(maxPoints));
            return grav_protocol::BackendJsonCodec::makeSnapshotResponse(hasSnapshot, snapshot);
        }
        if (cmd == grav_protocol::Pause) {
            _backend.setPaused(true);
            return grav_protocol::BackendJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == grav_protocol::Resume) {
            _backend.setPaused(false);
            return grav_protocol::BackendJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == grav_protocol::Toggle) {
            _backend.togglePaused();
            return grav_protocol::BackendJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == grav_protocol::Reset) {
            _backend.requestReset();
            return grav_protocol::BackendJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == grav_protocol::Recover) {
            _backend.requestRecover();
            return grav_protocol::BackendJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == grav_protocol::Step) {
            int count = 1;
            grav_protocol::BackendJsonCodec::readNumber(request, "count", count);
            if (count < 1) {
                count = 1;
            } else if (count > 100000) {
                count = 100000;
            }
            for (int i = 0; i < count; ++i) {
                _backend.stepOnce();
            }
            return grav_protocol::BackendJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == grav_protocol::SetDt) {
            double dt = 0.0;
            if (!grav_protocol::BackendJsonCodec::readNumber(request, "value", dt) || dt <= 0.0) {
                return grav_protocol::BackendJsonCodec::makeErrorResponse(cmd, "invalid dt value");
            }
            _backend.setDt(static_cast<float>(dt));
            return grav_protocol::BackendJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == grav_protocol::SetSolver) {
            std::string value;
            if (!grav_protocol::BackendJsonCodec::readString(request, "value", value)) {
                return grav_protocol::BackendJsonCodec::makeErrorResponse(cmd, "missing solver value");
            }
            std::string canonical;
            if (!grav_modes::normalizeSolver(value, canonical)) {
                return grav_protocol::BackendJsonCodec::makeErrorResponse(cmd, "invalid solver value");
            }
            _backend.setSolverMode(canonical);
            return grav_protocol::BackendJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == grav_protocol::SetIntegrator) {
            std::string value;
            if (!grav_protocol::BackendJsonCodec::readString(request, "value", value)) {
                return grav_protocol::BackendJsonCodec::makeErrorResponse(cmd, "missing integrator value");
            }
            std::string canonical;
            if (!grav_modes::normalizeIntegrator(value, canonical)) {
                return grav_protocol::BackendJsonCodec::makeErrorResponse(cmd, "invalid integrator value");
            }
            _backend.setIntegratorMode(canonical);
            return grav_protocol::BackendJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == grav_protocol::SetParticleCount) {
            std::uint64_t value = 0;
            if (!grav_protocol::BackendJsonCodec::readNumber(request, "value", value) || value < 2ull) {
                return grav_protocol::BackendJsonCodec::makeErrorResponse(cmd, "invalid particle count");
            }
            const std::uint64_t clamped = (value > 100000000ull) ? 100000000ull : value;
            _backend.setParticleCount(static_cast<std::uint32_t>(clamped));
            return grav_protocol::BackendJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == grav_protocol::SetSph) {
            bool value = false;
            if (!grav_protocol::BackendJsonCodec::readBool(request, "value", value)) {
                return grav_protocol::BackendJsonCodec::makeErrorResponse(cmd, "missing bool sph value");
            }
            _backend.setSphEnabled(value);
            return grav_protocol::BackendJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == grav_protocol::SetOctree) {
            double theta = 0.0;
            double softening = 0.0;
            if (!grav_protocol::BackendJsonCodec::readNumber(request, "theta", theta) || theta <= 0.0) {
                return grav_protocol::BackendJsonCodec::makeErrorResponse(cmd, "invalid theta");
            }
            if (!grav_protocol::BackendJsonCodec::readNumber(request, "softening", softening) || softening <= 0.0) {
                return grav_protocol::BackendJsonCodec::makeErrorResponse(cmd, "invalid softening");
            }
            _backend.setOctreeParameters(static_cast<float>(theta), static_cast<float>(softening));
            return grav_protocol::BackendJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == grav_protocol::SetSphParams) {
            double h = 0.0;
            double restDensity = 0.0;
            double gasConstant = 0.0;
            double viscosity = 0.0;
            if (!grav_protocol::BackendJsonCodec::readNumber(request, "h", h) || h <= 0.0
                || !grav_protocol::BackendJsonCodec::readNumber(request, "rest_density", restDensity) || restDensity <= 0.0
                || !grav_protocol::BackendJsonCodec::readNumber(request, "gas_constant", gasConstant) || gasConstant <= 0.0
                || !grav_protocol::BackendJsonCodec::readNumber(request, "viscosity", viscosity) || viscosity < 0.0) {
                return grav_protocol::BackendJsonCodec::makeErrorResponse(cmd, "invalid sph params");
            }
            _backend.setSphParameters(
                static_cast<float>(h),
                static_cast<float>(restDensity),
                static_cast<float>(gasConstant),
                static_cast<float>(viscosity));
            return grav_protocol::BackendJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == grav_protocol::SetEnergyMeasure) {
            std::uint32_t everySteps = 0;
            std::uint32_t sampleLimit = 0;
            if (!grav_protocol::BackendJsonCodec::readNumber(request, "every_steps", everySteps) || everySteps < 1u
                || !grav_protocol::BackendJsonCodec::readNumber(request, "sample_limit", sampleLimit) || sampleLimit < 2u) {
                return grav_protocol::BackendJsonCodec::makeErrorResponse(cmd, "invalid energy measure config");
            }
            _backend.setEnergyMeasurementConfig(everySteps, sampleLimit);
            return grav_protocol::BackendJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == grav_protocol::Load) {
            std::string path;
            if (!grav_protocol::BackendJsonCodec::readString(request, "path", path) || path.empty()) {
                return grav_protocol::BackendJsonCodec::makeErrorResponse(cmd, "missing path");
            }
            std::string format = "auto";
            grav_protocol::BackendJsonCodec::readString(request, "format", format);
            _backend.setInitialStateFile(path, format);
            _backend.requestReset();
            return grav_protocol::BackendJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == grav_protocol::Export) {
            std::string path;
            std::string format;
            grav_protocol::BackendJsonCodec::readString(request, "path", path);
            if (!grav_protocol::BackendJsonCodec::readString(request, "format", format)) {
                format = "vtk";
            }
            _backend.requestExportSnapshot(path, format);
            return grav_protocol::BackendJsonCodec::makeOkResponse(cmd);
        }
        if (cmd == grav_protocol::Shutdown) {
            _shutdownRequested.store(true);
            return grav_protocol::BackendJsonCodec::makeOkResponse(cmd);
        }

        return grav_protocol::BackendJsonCodec::makeErrorResponse(cmd, "unknown command");
    } catch (const std::exception &ex) {
        return grav_protocol::BackendJsonCodec::makeErrorResponse(
            "request",
            backendServerError("processRequest", ex.what()));
    } catch (...) {
        return grav_protocol::BackendJsonCodec::makeErrorResponse(
            "request",
            backendServerError("processRequest", "non-standard exception"));
    }
}
