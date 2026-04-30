# @file cmake/core/clang-format.cmake
# @author Luis1454
# @project BLITZAR
# @brief CMake build orchestration for BLITZAR targets and tooling.

# clang-format target for code style enforcement

set(BLITZAR_CLANG_FORMAT_EXECUTABLE "" CACHE FILEPATH "Path to clang-format executable")

if(NOT BLITZAR_CLANG_FORMAT_EXECUTABLE)
    find_program(BLITZAR_CLANG_FORMAT_EXECUTABLE clang-format)
endif()

if(NOT BLITZAR_CLANG_FORMAT_EXECUTABLE AND WIN32)
    set(_BLITZAR_default_clang_format "C:/Program Files/LLVM/bin/clang-format.exe")
    if(EXISTS "${_BLITZAR_default_clang_format}")
        set(BLITZAR_CLANG_FORMAT_EXECUTABLE "${_BLITZAR_default_clang_format}")
    endif()
endif()

if(BLITZAR_CLANG_FORMAT_EXECUTABLE)
    message(STATUS "clang-format found: ${BLITZAR_CLANG_FORMAT_EXECUTABLE}")

    add_custom_target(clang-format-check
        COMMAND ${CMAKE_COMMAND}
            -DMODE=check
            -DCLANG_FORMAT_EXECUTABLE=${BLITZAR_CLANG_FORMAT_EXECUTABLE}
            -DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
            -P ${CMAKE_CURRENT_LIST_DIR}/../clang-format-runner.cmake
        COMMENT "Checking code style with clang-format (no changes)"
    )

    add_custom_target(clang-format-fix
        COMMAND ${CMAKE_COMMAND}
            -DMODE=fix
            -DCLANG_FORMAT_EXECUTABLE=${BLITZAR_CLANG_FORMAT_EXECUTABLE}
            -DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
            -P ${CMAKE_CURRENT_LIST_DIR}/../clang-format-runner.cmake
        COMMENT "Applying clang-format to all source files"
    )
else()
    message(STATUS "clang-format not found. Install LLVM to enable formatting targets.")
endif()
