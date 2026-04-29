# @file cmake/rust.cmake
# @author Luis1454
# @project BLITZAR
# @brief CMake build orchestration for BLITZAR targets and tooling.

if(NOT BLITZAR_CARGO_EXECUTABLE)
    set(_BLITZAR_cargo_hints "")
    if(DEFINED ENV{CARGO_HOME})
        list(APPEND _BLITZAR_cargo_hints "$ENV{CARGO_HOME}/bin")
    endif()
    if(DEFINED ENV{USERPROFILE})
        list(APPEND _BLITZAR_cargo_hints "$ENV{USERPROFILE}/.cargo/bin")
    endif()
    if(_BLITZAR_cargo_hints)
        find_program(BLITZAR_CARGO_EXECUTABLE NAMES cargo cargo.exe HINTS ${_BLITZAR_cargo_hints} NO_DEFAULT_PATH)
    endif()
endif()

find_program(BLITZAR_CARGO_EXECUTABLE cargo REQUIRED)

if(DEFINED BLITZAR_ROOT_DIR)
    set(BLITZAR_RUST_ROOT_DIR "${BLITZAR_ROOT_DIR}")
else()
    set(BLITZAR_RUST_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
endif()

set(BLITZAR_RUST_WORKSPACE_DIR "${BLITZAR_RUST_ROOT_DIR}/rust")
set(BLITZAR_RUST_TARGET_DIR "${CMAKE_BINARY_DIR}/rust-target")
set(BLITZAR_RUST_RUNTIME_PROFILE "release")
set(
    BLITZAR_RUST_RUNTIME_LIBRARY
    "${BLITZAR_RUST_TARGET_DIR}/${BLITZAR_RUST_RUNTIME_PROFILE}/${CMAKE_STATIC_LIBRARY_PREFIX}blitzar_runtime${CMAKE_STATIC_LIBRARY_SUFFIX}"
)
set(
    BLITZAR_RUST_WEB_GATEWAY_BINARY
    "${BLITZAR_RUST_TARGET_DIR}/${BLITZAR_RUST_RUNTIME_PROFILE}/blitzar-web-gateway${CMAKE_EXECUTABLE_SUFFIX}"
)
set(
    BLITZAR_RUST_WEB_GATEWAY_OUTPUT
    "${CMAKE_BINARY_DIR}/blitzar-web-gateway${CMAKE_EXECUTABLE_SUFFIX}"
)
set(BLITZAR_RUST_RUNTIME_INPUTS
    "${BLITZAR_RUST_ROOT_DIR}/rust-toolchain.toml"
    "${BLITZAR_RUST_WORKSPACE_DIR}/Cargo.lock"
    "${BLITZAR_RUST_WORKSPACE_DIR}/Cargo.toml"
    "${BLITZAR_RUST_WORKSPACE_DIR}/blitzar-protocol/Cargo.toml"
    "${BLITZAR_RUST_WORKSPACE_DIR}/blitzar-protocol/src/codec.rs"
    "${BLITZAR_RUST_WORKSPACE_DIR}/blitzar-protocol/src/framing.rs"
    "${BLITZAR_RUST_WORKSPACE_DIR}/blitzar-protocol/src/lib.rs"
    "${BLITZAR_RUST_WORKSPACE_DIR}/blitzar-protocol/src/schema.rs"
    "${BLITZAR_RUST_WORKSPACE_DIR}/blitzar-protocol/src/version.rs"
    "${BLITZAR_RUST_WORKSPACE_DIR}/blitzar-protocol/src/v1/command.rs"
    "${BLITZAR_RUST_WORKSPACE_DIR}/blitzar-protocol/src/v1/envelope.rs"
    "${BLITZAR_RUST_WORKSPACE_DIR}/blitzar-protocol/src/v1/mod.rs"
    "${BLITZAR_RUST_WORKSPACE_DIR}/blitzar-protocol/src/v1/snapshot.rs"
    "${BLITZAR_RUST_WORKSPACE_DIR}/blitzar-protocol/src/v1/status.rs"
    "${BLITZAR_RUST_WORKSPACE_DIR}/blitzar-protocol/tests/protocol.rs"
    "${BLITZAR_RUST_WORKSPACE_DIR}/blitzar-runtime/Cargo.toml"
    "${BLITZAR_RUST_WORKSPACE_DIR}/blitzar-runtime/src/bridge_state.rs"
    "${BLITZAR_RUST_WORKSPACE_DIR}/blitzar-runtime/src/ffi.rs"
    "${BLITZAR_RUST_WORKSPACE_DIR}/blitzar-runtime/src/lib.rs"
    "${BLITZAR_RUST_WORKSPACE_DIR}/blitzar-runtime/tests/bridge_state.rs"
    "${BLITZAR_RUST_WORKSPACE_DIR}/blitzar-web-gateway/Cargo.toml"
    "${BLITZAR_RUST_WORKSPACE_DIR}/blitzar-web-gateway/src/api.rs"
    "${BLITZAR_RUST_WORKSPACE_DIR}/blitzar-web-gateway/src/args.rs"
    "${BLITZAR_RUST_WORKSPACE_DIR}/blitzar-web-gateway/src/backend.rs"
    "${BLITZAR_RUST_WORKSPACE_DIR}/blitzar-web-gateway/src/error.rs"
    "${BLITZAR_RUST_WORKSPACE_DIR}/blitzar-web-gateway/src/events.rs"
    "${BLITZAR_RUST_WORKSPACE_DIR}/blitzar-web-gateway/src/lib.rs"
    "${BLITZAR_RUST_WORKSPACE_DIR}/blitzar-web-gateway/src/main.rs"
    "${BLITZAR_RUST_WORKSPACE_DIR}/blitzar-web-gateway/src/ws.rs"
    "${BLITZAR_RUST_WORKSPACE_DIR}/blitzar-web-gateway/tests/args.rs"
    "${BLITZAR_RUST_WORKSPACE_DIR}/blitzar-web-gateway/tests/events.rs"
    "${BLITZAR_RUST_WORKSPACE_DIR}/blitzar-web-gateway/tests/http.rs"
)

function(BLITZAR_ensure_rust_runtime_target)
    if(TARGET blitzarRustRuntime)
        return()
    endif()

    add_custom_command(
        OUTPUT "${BLITZAR_RUST_RUNTIME_LIBRARY}"
        COMMAND ${BLITZAR_CARGO_EXECUTABLE}
            build
            --manifest-path "${BLITZAR_RUST_WORKSPACE_DIR}/Cargo.toml"
            -p blitzar-runtime
            --release
            --target-dir "${BLITZAR_RUST_TARGET_DIR}"
        DEPENDS ${BLITZAR_RUST_RUNTIME_INPUTS}
        WORKING_DIRECTORY "${BLITZAR_RUST_ROOT_DIR}"
        VERBATIM
    )
    add_custom_target(blitzarRustRuntimeBuild DEPENDS "${BLITZAR_RUST_RUNTIME_LIBRARY}")
    add_library(blitzarRustRuntime STATIC IMPORTED GLOBAL)
    set_target_properties(
        blitzarRustRuntime
        PROPERTIES
            IMPORTED_LOCATION "${BLITZAR_RUST_RUNTIME_LIBRARY}"
            INTERFACE_LINK_LIBRARIES "$<$<PLATFORM_ID:Windows>:ntdll;userenv>"
    )
    add_dependencies(blitzarRustRuntime blitzarRustRuntimeBuild)
endfunction()

function(BLITZAR_ensure_rust_web_gateway_target)
    if(TARGET blitzar-web-gateway)
        return()
    endif()

    add_custom_command(
        OUTPUT "${BLITZAR_RUST_WEB_GATEWAY_OUTPUT}"
        COMMAND ${BLITZAR_CARGO_EXECUTABLE}
            build
            --manifest-path "${BLITZAR_RUST_WORKSPACE_DIR}/Cargo.toml"
            -p blitzar-web-gateway
            --release
            --target-dir "${BLITZAR_RUST_TARGET_DIR}"
        COMMAND ${CMAKE_COMMAND}
            -E copy_if_different
            "${BLITZAR_RUST_WEB_GATEWAY_BINARY}"
            "${BLITZAR_RUST_WEB_GATEWAY_OUTPUT}"
        DEPENDS ${BLITZAR_RUST_RUNTIME_INPUTS}
        WORKING_DIRECTORY "${BLITZAR_RUST_ROOT_DIR}"
        VERBATIM
    )
    add_custom_target(blitzar-web-gateway ALL DEPENDS "${BLITZAR_RUST_WEB_GATEWAY_OUTPUT}")
endfunction()
