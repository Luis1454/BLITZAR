# @file cmake/dependencies.cmake
# @brief Automatic dependency detection and installation
# @description Cross-platform dependency management with auto-install capability

function(BLITZAR_find_windows_sdk_tool out_variable tool_name)
    find_program(_BLITZAR_TOOL_PATH NAMES ${tool_name} ${tool_name}.exe)
    if(NOT _BLITZAR_TOOL_PATH)
        set(_BLITZAR_SYSTEM_DRIVE "$ENV{SystemDrive}")
        if(NOT _BLITZAR_SYSTEM_DRIVE)
            set(_BLITZAR_SYSTEM_DRIVE "C:")
        endif()
        set(_BLITZAR_SDK_BIN "${_BLITZAR_SYSTEM_DRIVE}/Program Files (x86)/Windows Kits/10/bin")
        file(GLOB _BLITZAR_SDK_VERSIONS LIST_DIRECTORIES true "${_BLITZAR_SDK_BIN}/10.*")
        list(SORT _BLITZAR_SDK_VERSIONS COMPARE NATURAL ORDER DESCENDING)
        foreach(_BLITZAR_SDK_VERSION IN LISTS _BLITZAR_SDK_VERSIONS)
            if(IS_DIRECTORY "${_BLITZAR_SDK_VERSION}")
                foreach(_BLITZAR_SDK_ARCH x64 x86 arm64)
                    set(_BLITZAR_SDK_CANDIDATE "${_BLITZAR_SDK_VERSION}/${_BLITZAR_SDK_ARCH}/${tool_name}.exe")
                    if(EXISTS "${_BLITZAR_SDK_CANDIDATE}")
                        set(_BLITZAR_TOOL_PATH "${_BLITZAR_SDK_CANDIDATE}")
                        break()
                    endif()
                endforeach()
            endif()
            if(_BLITZAR_TOOL_PATH)
                break()
            endif()
        endforeach()
    endif()
    set(${out_variable} "${_BLITZAR_TOOL_PATH}" PARENT_SCOPE)
endfunction()

function(BLITZAR_check_and_install_deps)
    # Detect OS using standard Unix/Linux methods
    set(LINUX_DISTRO "")
    if(UNIX AND NOT APPLE)
        if(EXISTS "/etc/os-release")
            file(READ "/etc/os-release" OS_RELEASE_CONTENT)
            string(REGEX MATCH "ID=([^\n\r]*)" _ "${OS_RELEASE_CONTENT}")
            set(LINUX_DISTRO "${CMAKE_MATCH_1}")
            string(REPLACE "\"" "" LINUX_DISTRO "${LINUX_DISTRO}")
        endif()
    endif()

    message(STATUS "[DEPS] Detected: ${CMAKE_SYSTEM_NAME} / Distro: ${LINUX_DISTRO}")

    # Auto-install if requested on Linux
    if(BLITZAR_AUTO_INSTALL_DEPS AND UNIX AND NOT APPLE)
        message(STATUS "[AUTO-INSTALL] Attempting to install dependencies...")
        if(LINUX_DISTRO MATCHES "fedora|rhel|centos|rocky|alma")
            message(STATUS "[AUTO-INSTALL] Running: sudo dnf install ...")
            execute_process(
                COMMAND sudo dnf install -y gcc g++ gcc-c++ cmake ninja-build make
                TIMEOUT 300
                RESULT_VARIABLE DNF_RESULT
            )
            if(DNF_RESULT EQUAL 0)
                message(STATUS "[AUTO-INSTALL] ✓ Base dependencies installed")
            else()
                message(WARNING "[AUTO-INSTALL] ⚠ dnf install had non-zero exit code")
            endif()
        elseif(LINUX_DISTRO MATCHES "ubuntu|debian")
            message(STATUS "[AUTO-INSTALL] Running: sudo apt-get install ...")
            execute_process(
                COMMAND bash -c "sudo apt-get update && sudo apt-get install -y build-essential cmake ninja-build"
                TIMEOUT 300
                RESULT_VARIABLE APT_RESULT
            )
            if(APT_RESULT EQUAL 0)
                message(STATUS "[AUTO-INSTALL] ✓ Base dependencies installed")
            else()
                message(WARNING "[AUTO-INSTALL] ⚠ apt-get install had non-zero exit code")
            endif()
        else()
            message(WARNING "[AUTO-INSTALL] Unknown distro: ${LINUX_DISTRO}")
        endif()
    endif()

    # Check for tools (after potential installation)
    set(MISSING_TOOLS "")
    set(REQUIRED_TOOLS cmake)
    if(CMAKE_HOST_WIN32 OR WIN32)
        find_program(BLITZAR_WINDOWS_COMPILER NAMES cl clang-cl)
        if(BLITZAR_WINDOWS_COMPILER)
            message(STATUS "[DEPS] Windows compiler: ${BLITZAR_WINDOWS_COMPILER}")
        else()
            list(APPEND MISSING_TOOLS "cl or clang-cl")
        endif()
        foreach(tool rc mt)
            BLITZAR_find_windows_sdk_tool(TOOL_${tool}_PATH ${tool})
            if(TOOL_${tool}_PATH)
                message(STATUS "[DEPS] Windows tool ${tool}: ${TOOL_${tool}_PATH}")
                if(tool STREQUAL "rc")
                    set(CMAKE_RC_COMPILER "${TOOL_${tool}_PATH}" CACHE FILEPATH "Windows resource compiler" FORCE)
                else()
                    set(CMAKE_MT "${TOOL_${tool}_PATH}" CACHE FILEPATH "Windows manifest tool" FORCE)
                endif()
            else()
                list(APPEND MISSING_TOOLS ${tool})
            endif()
        endforeach()
    else()
        find_program(BLITZAR_C_COMPILER NAMES cc clang gcc)
        find_program(BLITZAR_CXX_COMPILER NAMES c++ clang++ g++)
        if(BLITZAR_C_COMPILER)
            message(STATUS "[DEPS] C compiler: ${BLITZAR_C_COMPILER}")
        else()
            list(APPEND MISSING_TOOLS "cc, clang or gcc")
        endif()
        if(BLITZAR_CXX_COMPILER)
            message(STATUS "[DEPS] CXX compiler: ${BLITZAR_CXX_COMPILER}")
        else()
            list(APPEND MISSING_TOOLS "c++, clang++ or g++")
        endif()
    endif()
    foreach(tool IN LISTS REQUIRED_TOOLS)
        find_program(TOOL_${tool}_PATH NAMES ${tool})
        if(TOOL_${tool}_PATH)
            message(STATUS "[DEPS] ✓ ${tool}: ${TOOL_${tool}_PATH}")
        else()
            message(STATUS "[DEPS] ✗ ${tool}: NOT FOUND")
            list(APPEND MISSING_TOOLS ${tool})
        endif()
        unset(TOOL_${tool}_PATH)
    endforeach()

    # Check ninja (optional)
    find_program(NINJA_PATH NAMES ninja)
    if(NINJA_PATH)
        message(STATUS "[DEPS] ✓ ninja available")
    else()
        message(STATUS "[DEPS] ⚠ ninja not available (using make)")
    endif()
    unset(NINJA_PATH)

    # Fatal error if missing critical tools
    if(MISSING_TOOLS)
        message(FATAL_ERROR
            "❌ Missing build tools: ${MISSING_TOOLS}\n\n"
            "On Fedora/RHEL:\n"
            "  sudo dnf install gcc gcc-c++ clang cmake ninja-build\n\n"
            "On Ubuntu/Debian:\n"
            "  sudo apt-get install build-essential clang cmake ninja-build\n\n"
            "On Windows, use a Visual Studio Developer PowerShell with cl.exe in PATH.\n\n"
            "Or with auto-install on Linux:\n"
            "  cmake -DBLITZAR_AUTO_INSTALL_DEPS=ON -S . -B build"
        )
    endif()

    message(STATUS "[DEPS] ✓ All build tools verified")
endfunction()

BLITZAR_check_and_install_deps()

