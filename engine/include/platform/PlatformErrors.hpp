#ifndef GRAVITY_SIM_PLATFORMERRORS_HPP
#define GRAVITY_SIM_PLATFORMERRORS_HPP

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

#endif // GRAVITY_SIM_PLATFORMERRORS_HPP
