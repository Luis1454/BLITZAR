find_program(GRAVITY_CARGO_EXECUTABLE cargo REQUIRED)

if(DEFINED GRAVITY_ROOT_DIR)
    set(GRAVITY_RUST_ROOT_DIR "${GRAVITY_ROOT_DIR}")
else()
    set(GRAVITY_RUST_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
endif()

set(GRAVITY_RUST_WORKSPACE_DIR "${GRAVITY_RUST_ROOT_DIR}/rust")
set(GRAVITY_RUST_TARGET_DIR "${CMAKE_BINARY_DIR}/rust-target")
set(GRAVITY_RUST_RUNTIME_PROFILE "release")
set(
    GRAVITY_RUST_RUNTIME_LIBRARY
    "${GRAVITY_RUST_TARGET_DIR}/${GRAVITY_RUST_RUNTIME_PROFILE}/${CMAKE_STATIC_LIBRARY_PREFIX}blitzar_runtime${CMAKE_STATIC_LIBRARY_SUFFIX}"
)
file(GLOB_RECURSE GRAVITY_RUST_RUNTIME_INPUTS CONFIGURE_DEPENDS
    "${GRAVITY_RUST_WORKSPACE_DIR}/Cargo.toml"
    "${GRAVITY_RUST_ROOT_DIR}/rust-toolchain.toml"
    "${GRAVITY_RUST_WORKSPACE_DIR}/Cargo.lock"
    "${GRAVITY_RUST_WORKSPACE_DIR}/blitzar-protocol/src/*.rs"
    "${GRAVITY_RUST_WORKSPACE_DIR}/blitzar-protocol/tests/*.rs"
    "${GRAVITY_RUST_WORKSPACE_DIR}/blitzar-runtime/src/*.rs"
    "${GRAVITY_RUST_WORKSPACE_DIR}/blitzar-runtime/tests/*.rs"
)

function(gravity_ensure_rust_runtime_target)
    if(TARGET gravityRustRuntime)
        return()
    endif()

    add_custom_command(
        OUTPUT "${GRAVITY_RUST_RUNTIME_LIBRARY}"
        COMMAND ${GRAVITY_CARGO_EXECUTABLE}
            build
            --manifest-path "${GRAVITY_RUST_WORKSPACE_DIR}/Cargo.toml"
            -p blitzar-runtime
            --release
            --target-dir "${GRAVITY_RUST_TARGET_DIR}"
        DEPENDS ${GRAVITY_RUST_RUNTIME_INPUTS}
        WORKING_DIRECTORY "${GRAVITY_RUST_ROOT_DIR}"
        VERBATIM
    )
    add_custom_target(gravityRustRuntimeBuild DEPENDS "${GRAVITY_RUST_RUNTIME_LIBRARY}")
    add_library(gravityRustRuntime STATIC IMPORTED GLOBAL)
    set_target_properties(
        gravityRustRuntime
        PROPERTIES
            IMPORTED_LOCATION "${GRAVITY_RUST_RUNTIME_LIBRARY}"
            INTERFACE_LINK_LIBRARIES "$<$<PLATFORM_ID:Windows>:ntdll;userenv>"
    )
    add_dependencies(gravityRustRuntime gravityRustRuntimeBuild)
endfunction()
