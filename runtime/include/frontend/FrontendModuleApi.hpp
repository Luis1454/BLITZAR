#pragma once

#include <cstddef>
#include <cstdint>

#define GRAVITY_FRONTEND_MODULE_EXPORT GRAVITY_FRONTEND_MODULE_EXPORT_ATTR

namespace grav_module {

extern const std::uint32_t kFrontendModuleApiVersionV1;
extern const char *kFrontendModuleEntryPoint;

struct FrontendModuleHostContextV1 {
    const char *configPath;
};

typedef bool (*FrontendModuleCreateFn)(
    const FrontendModuleHostContextV1 *context,
    void **outModuleState,
    char *errorBuffer,
    std::size_t errorBufferSize);
typedef void (*FrontendModuleDestroyFn)(void *moduleState);
typedef bool (*FrontendModuleStartFn)(void *moduleState, char *errorBuffer, std::size_t errorBufferSize);
typedef void (*FrontendModuleStopFn)(void *moduleState);
typedef bool (*FrontendModuleHandleCommandFn)(
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

typedef const FrontendModuleExportsV1 *(*FrontendModuleEntryPointFn)();
} // namespace grav_module

