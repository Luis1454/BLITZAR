#include <cstddef>
#include <exception>
#include <iostream>
#include <memory>

#include "frontend/ErrorBuffer.hpp"
#include "modules/cli/module_cli_lifecycle.hpp"
#include "modules/cli/module_cli_state.hpp"

namespace grav_module_cli {

class ModuleCliLifecycleLocal final {
public:
    static bool create(
        const grav_module::FrontendModuleHostContextV1 *,
        void **outModuleState,
        char *errorBuffer,
        std::size_t errorBufferSize)
    {
        try {
            if (outModuleState == nullptr) {
                grav_frontend::writeErrorBuffer(errorBuffer, errorBufferSize, "outModuleState is null");
                return false;
            }
            std::unique_ptr<ModuleState> state = std::make_unique<ModuleState>();
            state->client.setSocketTimeoutMs(150);
            *outModuleState = state.release();
            return true;
        } catch (const std::exception &ex) {
            grav_frontend::writeErrorBuffer(errorBuffer, errorBufferSize, ex.what());
            return false;
        } catch (...) {
            grav_frontend::writeErrorBuffer(errorBuffer, errorBufferSize, "unknown module create error");
            return false;
        }
    }

    static void destroy(void *moduleState)
    {
        try {
            ModuleState *state = static_cast<ModuleState *>(moduleState);
            if (state != nullptr) {
                std::unique_ptr<ModuleState> ownedState(state);
                ownedState->client.disconnect();
            }
        } catch (const std::exception &ex) {
            std::cerr << "[module-cli] destroy error: " << ex.what() << "\n";
        } catch (...) {
            std::cerr << "[module-cli] destroy error: unknown\n";
        }
    }

    static bool start(void *moduleState, char *errorBuffer, std::size_t errorBufferSize)
    {
        try {
            ModuleState *state = static_cast<ModuleState *>(moduleState);
            if (state == nullptr) {
                grav_frontend::writeErrorBuffer(errorBuffer, errorBufferSize, "module state is null");
                return false;
            }
            if (!state->client.connect(state->host, state->port)) {
                std::cout << "[module-cli] startup: backend " << state->host << ":" << state->port
                          << " not reachable yet (retry on command)\n";
            } else {
                std::cout << "[module-cli] connected to " << state->host << ":" << state->port << "\n";
            }
            return true;
        } catch (const std::exception &ex) {
            grav_frontend::writeErrorBuffer(errorBuffer, errorBufferSize, ex.what());
            return false;
        } catch (...) {
            grav_frontend::writeErrorBuffer(errorBuffer, errorBufferSize, "unknown module start error");
            return false;
        }
    }

    static void stop(void *moduleState)
    {
        try {
            ModuleState *state = static_cast<ModuleState *>(moduleState);
            if (state != nullptr) {
                state->client.disconnect();
            }
        } catch (const std::exception &ex) {
            std::cerr << "[module-cli] stop error: " << ex.what() << "\n";
        } catch (...) {
            std::cerr << "[module-cli] stop error: unknown\n";
        }
    }
};

bool ModuleCliLifecycle::create(
    const grav_module::FrontendModuleHostContextV1 *hostContext,
    void **outModuleState,
    char *errorBuffer,
    std::size_t errorBufferSize)
{
    return ModuleCliLifecycleLocal::create(hostContext, outModuleState, errorBuffer, errorBufferSize);
}

void ModuleCliLifecycle::destroy(void *moduleState)
{
    ModuleCliLifecycleLocal::destroy(moduleState);
}

bool ModuleCliLifecycle::start(void *moduleState, char *errorBuffer, std::size_t errorBufferSize)
{
    return ModuleCliLifecycleLocal::start(moduleState, errorBuffer, errorBufferSize);
}

void ModuleCliLifecycle::stop(void *moduleState)
{
    ModuleCliLifecycleLocal::stop(moduleState);
}

} // namespace grav_module_cli
