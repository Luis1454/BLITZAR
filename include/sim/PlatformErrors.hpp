#ifndef GRAVITY_SIM_PLATFORMERRORS_HPP
#define GRAVITY_SIM_PLATFORMERRORS_HPP

#include <string_view>

namespace sim::platform::errors {

inline constexpr std::string_view kInvalidLibraryHandleOrSymbol = "invalid library handle or symbol name";
inline constexpr std::string_view kDynamicLibraryLoadFailed = "dynamic library load failed";
inline constexpr std::string_view kMissingSymbolPrefix = "missing symbol: ";
inline constexpr std::string_view kProcessLaunchFailed = "process launch failed";
inline constexpr std::string_view kProcessTerminateFailed = "failed to terminate process";
inline constexpr std::string_view kUnexpectedException = "unexpected exception";
inline constexpr std::string_view kUnknownException = "unknown exception";

} // namespace sim::platform::errors

#endif // GRAVITY_SIM_PLATFORMERRORS_HPP
