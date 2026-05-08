# @file cmake/dependencies.cmake
# @brief Automatic dependency detection and installation
# @description Cross-platform dependency management with auto-install capability

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
    foreach(tool gcc g++ cmake)
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
            "  sudo dnf install gcc g++ gcc-c++ cmake ninja-build\n\n"
            "On Ubuntu/Debian:\n"
            "  sudo apt-get install build-essential cmake ninja-build\n\n"
            "Or with auto-install:\n"
            "  cmake -DBLITZAR_AUTO_INSTALL_DEPS=ON -S . -B build"
        )
    endif()

    message(STATUS "[DEPS] ✓ All build tools verified")
endfunction()

BLITZAR_check_and_install_deps()


# Call this early in main CMakeLists.txt
BLITZAR_check_and_install_deps()

