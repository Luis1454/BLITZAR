cmake_minimum_required(VERSION 3.25)

project(BLITZAR VERSION 1.0.0 LANGUAGES C CXX)

include(GNUInstallDirs)

option(BLITZAR_BUILD_TESTS "Build BLITZAR tests" ON)
option(BLITZAR_BUILD_CLI "Build the BLITZAR command-line application" ON)
option(BLITZAR_BUILD_EXAMPLES "Build SDK consumption examples" ON)

function(blitzar_enable_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /WX)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
endfunction()

add_library(blitzar STATIC
    src/integration/LeapfrogKdk.cpp
    src/particles/ParticleArray.cpp
    src/particles/ParticleBuffer.cpp
    src/physics/GravityLaw.cpp
    src/sdk/Api.cpp
    src/sdk/Context.cpp
    src/solvers/direct/DirectSolver.cpp
)

target_compile_features(blitzar PUBLIC cxx_std_20)
target_include_directories(blitzar
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

blitzar_enable_warnings(blitzar)

install(TARGETS blitzar
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES
    include/blitzar/blitzar.h
    include/blitzar/blitzar.hpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/blitzar
)

if(BLITZAR_BUILD_CLI)
    add_executable(blitzar_cli
        apps/blitzar/main.cpp
    )
    target_link_libraries(blitzar_cli PRIVATE blitzar)
    target_compile_features(blitzar_cli PRIVATE cxx_std_20)
    blitzar_enable_warnings(blitzar_cli)
    install(TARGETS blitzar_cli RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

if(BLITZAR_BUILD_EXAMPLES)
    add_executable(blitzar_c_example
        examples/CExample.c
    )
    target_link_libraries(blitzar_c_example PRIVATE blitzar)
    set_property(TARGET blitzar_c_example PROPERTY LINKER_LANGUAGE CXX)
    blitzar_enable_warnings(blitzar_c_example)

    add_executable(blitzar_cpp_example
        examples/CppExample.cpp
    )
    target_link_libraries(blitzar_cpp_example PRIVATE blitzar)
    target_compile_features(blitzar_cpp_example PRIVATE cxx_std_20)
    blitzar_enable_warnings(blitzar_cpp_example)
endif()

if(BLITZAR_BUILD_TESTS)
    enable_testing()

    add_executable(blitzar_lifecycle_test
        tests/Lifecycle.cpp
    )
    target_link_libraries(blitzar_lifecycle_test PRIVATE blitzar)
    target_compile_features(blitzar_lifecycle_test PRIVATE cxx_std_20)
    blitzar_enable_warnings(blitzar_lifecycle_test)

    add_executable(blitzar_c_api_test
        tests/ApiC.c
    )
    target_link_libraries(blitzar_c_api_test PRIVATE blitzar)
    set_property(TARGET blitzar_c_api_test PROPERTY LINKER_LANGUAGE CXX)
    blitzar_enable_warnings(blitzar_c_api_test)

    add_executable(blitzar_contract_test
        tests/Contracts.cpp
    )
    target_link_libraries(blitzar_contract_test PRIVATE blitzar)
    target_compile_features(blitzar_contract_test PRIVATE cxx_std_20)
    target_include_directories(blitzar_contract_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    blitzar_enable_warnings(blitzar_contract_test)

    add_executable(blitzar_dynamics_test
        tests/Dynamics.cpp
    )
    target_link_libraries(blitzar_dynamics_test PRIVATE blitzar)
    target_compile_features(blitzar_dynamics_test PRIVATE cxx_std_20)
    target_include_directories(blitzar_dynamics_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    blitzar_enable_warnings(blitzar_dynamics_test)

    add_test(NAME TST-P0-001-cpp-lifecycle COMMAND blitzar_lifecycle_test)
    add_test(NAME TST-P0-002-c-api-lifecycle COMMAND blitzar_c_api_test)
    add_test(NAME TST-P0-003-core-contracts COMMAND blitzar_contract_test)
    add_test(NAME TST-P1-001-dynamics COMMAND blitzar_dynamics_test)
endif()

if(BLITZAR_BUILD_TESTS)
    if(BLITZAR_BUILD_EXAMPLES)
        add_test(NAME TST-P2-001-c-example COMMAND blitzar_c_example)
        add_test(NAME TST-P2-002-cpp-example COMMAND blitzar_cpp_example)
    endif()
    if(BLITZAR_BUILD_CLI)
        add_test(NAME TST-P2-003-cli COMMAND blitzar_cli)
    endif()
endif()
