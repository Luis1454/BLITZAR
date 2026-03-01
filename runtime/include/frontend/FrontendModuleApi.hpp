#ifndef GRAVITY_SIM_FRONTENDMODULEAPI_HPP
#define GRAVITY_SIM_FRONTENDMODULEAPI_HPP

#include <cstddef>
#include <cstdint>

#if defined(_WIN32)
#define GRAVITY_FRONTEND_MODULE_EXPORT extern "C" __declspec(dllexport)
#else
#define GRAVITY_FRONTEND_MODULE_EXPORT extern "C"
#endif

namespace grav_module {

extern const std::uint32_t kFrontendModuleApiVersionV1;
extern const char *kFrontendModuleEntryPoint;

struct FrontendModuleHostContextV1 {
    const char *configPath;
};

typedef bool (*)(
    const FrontendModuleHostContextV1 *context,
    void **outModuleState,
    char *errorBuffer,
    std::size_t errorBufferSize) FrontendModuleCreateFn;
typedef void (*)(void *moduleState) FrontendModuleDestroyFn;
typedef bool (*)(void *moduleState, char *errorBuffer, std::size_t errorBufferSize) FrontendModuleStartFn;
typedef void (*)(void *moduleState) FrontendModuleStopFn;
typedef bool (*)(
    void *moduleState,
    const char *commandLine,
    bool *outKeepRunning,
    char *errorBuffer,
    std::size_t errorBufferSize) FrontendModuleHandleCommandFn;
struct FrontendModuleExportsV1 {
    std::uint32_t apiVersion;
    const char *moduleName;
    FrontendModuleCreateFn create;
    FrontendModuleDestroyFn destroy;
    FrontendModuleStartFn start;
    FrontendModuleStopFn stop;
    FrontendModuleHandleCommandFn handleCommand;
};

typedef const FrontendModuleExportsV1 *(*)() FrontendModuleEntryPointFn;
} // namespace grav_module

#endif // GRAVITY_SIM_FRONTENDMODULEAPI_HPP
