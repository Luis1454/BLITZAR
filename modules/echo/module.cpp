#include "frontend/FrontendModuleApi.hpp"
#include "frontend/FrontendModuleBoundary.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <exception>
#include <iostream>
#include <memory>
#include <string>

static std::string trim(const std::string &input)
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

class EchoModuleLocal final {
public:
    static bool create(
        const grav_module::FrontendModuleHostContextV1 *context,
        const grav_module::FrontendModuleStateSlot &outModuleState,
        const grav_frontend::ErrorBufferView &errorBuffer)
    {
        try {
            if (!outModuleState.isAvailable()) {
                errorBuffer.write("outModuleState is null");
                return false;
            }
            std::unique_ptr<EchoState> state = std::make_unique<EchoState>();
            state->configPath = (context != nullptr && context->configPath != nullptr) ? context->configPath : "simulation.ini";
            return outModuleState.assign(grav_module::FrontendModuleOpaqueState::fromRawPointer(state.release()));
        } catch (const std::exception &ex) {
            errorBuffer.write(ex.what());
            return false;
        } catch (...) {
            errorBuffer.write("unknown module create error");
            return false;
        }
    }

    static void destroy(grav_module::FrontendModuleOpaqueState moduleState)
    {
        try {
            std::unique_ptr<EchoState> state(static_cast<EchoState *>(moduleState.rawPointer()));
        } catch (const std::exception &ex) {
            std::cerr << "[module-echo] destroy error: " << ex.what() << "\n";
        } catch (...) {
            std::cerr << "[module-echo] destroy error: unknown\n";
        }
    }

    static bool start(
        grav_module::FrontendModuleOpaqueState moduleState,
        const grav_frontend::ErrorBufferView &errorBuffer)
    {
        try {
            EchoState *state = static_cast<EchoState *>(moduleState.rawPointer());
            if (state == nullptr) {
                errorBuffer.write("module state is null");
                return false;
            }
            std::cout << "[module-echo] started (config=" << state->configPath << ")\n";
            return true;
        } catch (const std::exception &ex) {
            errorBuffer.write(ex.what());
            return false;
        } catch (...) {
            errorBuffer.write("unknown module start error");
            return false;
        }
    }

    static bool handleCommand(
        std::string_view commandLine,
        const grav_module::FrontendModuleCommandControl &commandControl,
        const grav_frontend::ErrorBufferView &errorBuffer)
    {
        try {
            commandControl.setContinue();
            const std::string line = trim(std::string(commandLine));
            if (line.empty()) {
                return true;
            }
            if (line == "quit" || line == "exit") {
                commandControl.requestStop();
                return true;
            }
            std::cout << "[module-echo] " << line << "\n";
            return true;
        } catch (const std::exception &ex) {
            errorBuffer.write(ex.what());
            return false;
        } catch (...) {
            errorBuffer.write("unknown module command error");
            return false;
        }
    }
};

extern "C" GRAVITY_FRONTEND_MODULE_EXPORT const grav_module::FrontendModuleExportsV1 *gravity_frontend_module_v1()
{
    static const grav_module::FrontendModuleExportsV1 exports{
        grav_module::kFrontendModuleApiVersionV1,
        "echo-module",
        [](const grav_module::FrontendModuleHostContextV1 *context, void **outModuleState, char *errorBuffer, std::size_t errorBufferSize) -> bool {
            return EchoModuleLocal::create(
                context,
                grav_module::FrontendModuleStateSlot(outModuleState),
                grav_frontend::ErrorBufferView(errorBuffer, errorBufferSize));
        },
        [](void *moduleState) {
            EchoModuleLocal::destroy(grav_module::FrontendModuleOpaqueState::fromRawPointer(moduleState));
        },
        [](void *moduleState, char *errorBuffer, std::size_t errorBufferSize) -> bool {
            return EchoModuleLocal::start(
                grav_module::FrontendModuleOpaqueState::fromRawPointer(moduleState),
                grav_frontend::ErrorBufferView(errorBuffer, errorBufferSize));
        },
        [](void *) {
            try {
                std::cout << "[module-echo] stopped\n";
            } catch (...) {
            }
        },
        [](void *, const char *commandLine, bool *outKeepRunning, char *errorBuffer, std::size_t errorBufferSize) -> bool {
            return EchoModuleLocal::handleCommand(
                commandLine != nullptr ? std::string_view(commandLine) : std::string_view(),
                grav_module::FrontendModuleCommandControl(outKeepRunning),
                grav_frontend::ErrorBufferView(errorBuffer, errorBufferSize));
        }
    };
    return &exports;
}

