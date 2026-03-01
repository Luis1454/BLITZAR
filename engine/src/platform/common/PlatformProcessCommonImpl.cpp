#include "platform/common/PlatformProcessCommonImpl.hpp"
#include "platform/PlatformErrors.hpp"

#include "platform/internal/ProcessOps.hpp"

#include <exception>
#include <utility>

namespace grav_platform {

struct ProcessHandle::Impl {
    grav_platform_detail::NativeProcessHandle nativeHandle = 0u;
    std::int64_t pid = 0;
    std::string commandLine;
};

ProcessHandle::ProcessHandle()
    : _impl(std::make_unique<Impl>())
{
}

ProcessHandle::~ProcessHandle() = default;

ProcessHandle::ProcessHandle(ProcessHandle &&other) noexcept
    : _impl(std::move(other._impl))
{
}

ProcessHandle &ProcessHandle::operator=(ProcessHandle &&other) noexcept
{
    if (this != &other) {
        _impl = std::move(other._impl);
    }
    return *this;
}

bool ProcessHandle::launch(
    const std::string &executable,
    const std::vector<std::string> &args,
    bool createNewConsole,
    std::string &outError)
{
    try {
        if (!_impl) {
            _impl = std::make_unique<Impl>();
        }
        clear();
        _impl->commandLine = buildProcessCommandLine(executable, args);
        if (!grav_platform_detail::launchProcess(
                executable,
                args,
                createNewConsole,
                _impl->nativeHandle,
                _impl->pid,
                outError)) {
            _impl->commandLine.clear();
            if (outError.empty()) {
                outError = grav_platform_errors::kProcessLaunchFailed;
            }
            return false;
        }
        return true;
    } catch (const std::exception &ex) {
        outError = ex.what();
        return false;
    } catch (...) {
        outError = grav_platform_errors::kUnknownException;
        return false;
    }
}

bool ProcessHandle::terminate(std::uint32_t waitMs, std::string &outError)
{
    try {
        if (!isRunning()) {
            clear();
            return true;
        }
        if (!_impl) {
            return true;
        }
        if (!grav_platform_detail::terminateProcess(_impl->nativeHandle, _impl->pid, waitMs, outError)) {
            if (outError.empty()) {
                outError = grav_platform_errors::kProcessTerminateFailed;
            }
            return false;
        }
        _impl->commandLine.clear();
        return true;
    } catch (const std::exception &ex) {
        outError = ex.what();
        return false;
    } catch (...) {
        outError = grav_platform_errors::kUnknownException;
        return false;
    }
}

bool ProcessHandle::isRunning() const
{
    return _impl && grav_platform_detail::isProcessRunning(_impl->nativeHandle, _impl->pid);
}

void ProcessHandle::clear()
{
    if (!_impl) {
        return;
    }
    grav_platform_detail::clearProcessHandle(_impl->nativeHandle, _impl->pid);
    _impl->commandLine.clear();
}

std::string ProcessHandle::pidString() const
{
    if (!_impl) {
        return {};
    }
    return grav_platform_detail::formatProcessId(_impl->pid);
}

const std::string &ProcessHandle::commandLine() const
{
    static const std::string kEmpty;
    if (!_impl) {
        return kEmpty;
    }
    return _impl->commandLine;
}

bool launchDetachedProcess(const std::string &executable, const std::vector<std::string> &args, std::string &outError)
{
    try {
        if (!grav_platform_detail::launchDetachedProcess(executable, args, outError)) {
            if (outError.empty()) {
                outError = grav_platform_errors::kProcessLaunchFailed;
            }
            return false;
        }
        return true;
    } catch (const std::exception &ex) {
        outError = ex.what();
        return false;
    } catch (...) {
        outError = grav_platform_errors::kUnknownException;
        return false;
    }
}

int runProcessBlocking(
    const std::string &executable,
    const std::vector<std::string> &args,
    bool createNewConsole,
    std::string &outError)
{
    try {
        return grav_platform_detail::runProcessBlocking(executable, args, createNewConsole, outError);
    } catch (const std::exception &ex) {
        outError = ex.what();
        return 1;
    } catch (...) {
        outError = grav_platform_errors::kUnknownException;
        return 1;
    }
}

} // namespace grav_platform



