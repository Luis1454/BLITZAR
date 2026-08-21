cmake_minimum_required(VERSION 3.25)

project(BLITZAR VERSION 1.0.0 LANGUAGES C CXX)

include(GNUInstallDirs)

option(BLITZAR_BUILD_TESTS "Build BLITZAR tests" ON)

add_library(blitzar STATIC
    src/sdk/Api.cpp
    src/sdk/Context.cpp
)

target_compile_features(blitzar PUBLIC cxx_std_20)
target_include_directories(blitzar
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(MSVC)
    target_compile_options(blitzar PRIVATE /W4 /WX)
else()
    target_compile_options(blitzar PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

install(TARGETS blitzar
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES
    include/blitzar/blitzar.h
    include/blitzar/blitzar.hpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/blitzar
)

if(BLITZAR_BUILD_TESTS)
    enable_testing()

    add_executable(blitzar_lifecycle_test
        tests/Lifecycle.cpp
    )
    target_link_libraries(blitzar_lifecycle_test PRIVATE blitzar)
    target_compile_features(blitzar_lifecycle_test PRIVATE cxx_std_20)

    add_executable(blitzar_c_api_test
        tests/ApiC.c
    )
    target_link_libraries(blitzar_c_api_test PRIVATE blitzar)
    set_property(TARGET blitzar_c_api_test PROPERTY LINKER_LANGUAGE CXX)

    add_test(NAME TST-P0-001-cpp-lifecycle COMMAND blitzar_lifecycle_test)
    add_test(NAME TST-P0-002-c-api-lifecycle COMMAND blitzar_c_api_test)
endif()
