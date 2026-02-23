#ifndef GRAVITY_SIM_FRONTENDMODULEAPI_HPP
#define GRAVITY_SIM_FRONTENDMODULEAPI_HPP

#include <cstddef>
#include <cstdint>

#if defined(_WIN32)
#define GRAVITY_FRONTEND_MODULE_EXPORT extern "C" __declspec(dllexport)
#else
#define GRAVITY_FRONTEND_MODULE_EXPORT extern "C"
#endif

namespace sim::module {

constexpr std::uint32_t kFrontendModuleApiVersionV1 = 1u;
constexpr const char *kFrontendModuleEntryPoint = "gravity_frontend_module_v1";

struct FrontendModuleHostContextV1 {
    const char *configPath;
};

using FrontendModuleCreateFn = bool (*)(
    const FrontendModuleHostContextV1 *context,
    void **outModuleState,
    char *errorBuffer,
    std::size_t errorBufferSize);
using FrontendModuleDestroyFn = void (*)(void *moduleState);
using FrontendModuleStartFn = bool (*)(void *moduleState, char *errorBuffer, std::size_t errorBufferSize);
using FrontendModuleStopFn = void (*)(void *moduleState);
using FrontendModuleHandleCommandFn = bool (*)(
    void *moduleState,
    const char *commandLine,
    bool *outKeepRunning,
    char *errorBuffer,
    std::size_t errorBufferSize);

struct FrontendModuleExportsV1 {
    std::uint32_t apiVersion;
    const char *moduleName;
    FrontendModuleCreateFn create;
    FrontendModuleDestroyFn destroy;
    FrontendModuleStartFn start;
    FrontendModuleStopFn stop;
    FrontendModuleHandleCommandFn handleCommand;
};

using FrontendModuleEntryPointFn = const FrontendModuleExportsV1 *(*)();

} // namespace sim::module

#endif // GRAVITY_SIM_FRONTENDMODULEAPI_HPP
