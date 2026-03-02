#include "frontend/ErrorBuffer.hpp"
#include "frontend/FrontendModuleApi.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <exception>
#include <iostream>
#include <memory>
#include <string>

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

struct EchoState {
    std::string configPath;
};
GRAVITY_FRONTEND_MODULE_EXPORT const grav_module::FrontendModuleExportsV1 *gravity_frontend_module_v1()
{
    static const grav_module::FrontendModuleExportsV1 exports{
        grav_module::kFrontendModuleApiVersionV1,
        "echo-module",
        [](const grav_module::FrontendModuleHostContextV1 *context, void **outModuleState, char *errorBuffer, std::size_t errorBufferSize) -> bool {
            try {
                if (outModuleState == nullptr) {
                    grav_frontend::writeErrorBuffer(errorBuffer, errorBufferSize, "outModuleState is null");
                    return false;
                }
                std::unique_ptr<EchoState> state = std::make_unique<EchoState>();
                state->configPath = (context != nullptr && context->configPath != nullptr) ? context->configPath : "simulation.ini";
                *outModuleState = state.release();
                return true;
            } catch (const std::exception &ex) {
                grav_frontend::writeErrorBuffer(errorBuffer, errorBufferSize, ex.what());
                return false;
            } catch (...) {
                grav_frontend::writeErrorBuffer(errorBuffer, errorBufferSize, "unknown module create error");
                return false;
            }
        },
        [](void *moduleState) {
            try {
                std::unique_ptr<EchoState> state(static_cast<EchoState *>(moduleState));
            } catch (const std::exception &ex) {
                std::cerr << "[module-echo] destroy error: " << ex.what() << "\n";
            } catch (...) {
                std::cerr << "[module-echo] destroy error: unknown\n";
            }
        },
        [](void *moduleState, char *errorBuffer, std::size_t errorBufferSize) -> bool {
            try {
                auto *state = static_cast<EchoState *>(moduleState);
                if (state == nullptr) {
                    grav_frontend::writeErrorBuffer(errorBuffer, errorBufferSize, "module state is null");
                    return false;
                }
                std::cout << "[module-echo] started (config=" << state->configPath << ")\n";
                return true;
            } catch (const std::exception &ex) {
                grav_frontend::writeErrorBuffer(errorBuffer, errorBufferSize, ex.what());
                return false;
            } catch (...) {
                grav_frontend::writeErrorBuffer(errorBuffer, errorBufferSize, "unknown module start error");
                return false;
            }
        },
        [](void *) {
            try {
                std::cout << "[module-echo] stopped\n";
            } catch (...) {
            }
        },
        [](void *, const char *commandLine, bool *outKeepRunning, char *errorBuffer, std::size_t errorBufferSize) -> bool {
            try {
                if (outKeepRunning != nullptr) {
                    *outKeepRunning = true;
                }
                const std::string line = trim(commandLine != nullptr ? std::string(commandLine) : std::string());
                if (line.empty()) {
                    return true;
                }
                if (line == "quit" || line == "exit") {
                    if (outKeepRunning != nullptr) {
                        *outKeepRunning = false;
                    }
                    return true;
                }
                std::cout << "[module-echo] " << line << "\n";
                return true;
            } catch (const std::exception &ex) {
                grav_frontend::writeErrorBuffer(errorBuffer, errorBufferSize, ex.what());
                return false;
            } catch (...) {
                grav_frontend::writeErrorBuffer(errorBuffer, errorBufferSize, "unknown module command error");
                return false;
            }
        }
    };
    return &exports;
}

