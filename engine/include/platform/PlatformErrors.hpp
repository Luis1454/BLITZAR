#ifndef GRAVITY_ENGINE_INCLUDE_PLATFORM_PLATFORMERRORS_HPP_
#define GRAVITY_ENGINE_INCLUDE_PLATFORM_PLATFORMERRORS_HPP_
#include <string_view>
namespace grav_platform_errors {
extern const std::string_view kInvalidLibraryHandleOrSymbol;
extern const std::string_view kDynamicLibraryLoadFailed;
extern const std::string_view kMissingSymbolPrefix;
extern const std::string_view kProcessLaunchFailed;
extern const std::string_view kProcessTerminateFailed;
extern const std::string_view kUnexpectedException;
extern const std::string_view kUnknownException;
} // namespace grav_platform_errors
#endif // GRAVITY_ENGINE_INCLUDE_PLATFORM_PLATFORMERRORS_HPP_
