#include "fixtures/FixtureProcess.hpp"

#include <cstdlib>
#include <new>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#ifdef _WIN32

#include <windows.h>

#endif

namespace blitzar_test {

namespace {

#ifdef _WIN32

[[nodiscard]] std::wstring BuildCommand(const std::filesystem::path& executable,
    std::string_view mode, const std::filesystem::path& argument)
{
    const std::wstring wide_mode(mode.begin(), mode.end());

    return L"\"" + executable.native() + L"\" \"" + wide_mode + L"\" \"" + argument.native() +
           L"\"";
}

#else

[[nodiscard]] std::string Quote(std::string_view value)
{
    std::string quoted{"'"};

    for (const char character : value) {
        if (character == '\'') {
            quoted += "'\\''";
        }

        quoted += character;
    }

    quoted += '\'';

    return quoted;
}

#endif

} // namespace

bool RunProcess(const std::filesystem::path& executable, std::string_view mode,
    const std::filesystem::path& argument) noexcept
{
    try {
#ifdef _WIN32
        const std::wstring command = BuildCommand(executable, mode, argument);
        std::vector<wchar_t> command_line(command.begin(), command.end());

        command_line.push_back(L'\0');

        STARTUPINFOW startup{};

        startup.cb = sizeof(startup);

        PROCESS_INFORMATION process{};

        if (CreateProcessW(nullptr, command_line.data(), nullptr, nullptr, FALSE, 0, nullptr,
                nullptr, &startup, &process) == FALSE) {
            return false;
        }

        const DWORD wait_result = WaitForSingleObject(process.hProcess, INFINITE);
        DWORD exit_code = 1U;
        const bool succeeded = wait_result == WAIT_OBJECT_0 &&
                               GetExitCodeProcess(process.hProcess, &exit_code) != FALSE &&
                               exit_code == 0U;

        (void)CloseHandle(process.hThread);
        (void)CloseHandle(process.hProcess);

        return succeeded;
#else

        const std::string command =
            Quote(executable.string()) + " " + Quote(mode) + " " + Quote(argument.string());

        return std::system(command.c_str()) == 0;
#endif
    }
    catch (const std::bad_alloc&) {
        return false;
    }
    catch (const std::length_error&) {
        return false;
    }
}

} // namespace blitzar_test
