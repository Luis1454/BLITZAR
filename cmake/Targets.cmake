if(BLITZAR_BUILD_CLI)
    add_executable(blitzar_cli
        apps/blitzar/Main.cpp
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

    add_executable(blitzar_c_v2_example
        examples/CV2Example.c
    )
    target_link_libraries(blitzar_c_v2_example PRIVATE blitzar)
    set_property(TARGET blitzar_c_v2_example PROPERTY LINKER_LANGUAGE CXX)
    blitzar_enable_warnings(blitzar_c_v2_example)

    add_executable(blitzar_cpp_example
        examples/CppExample.cpp
    )
    target_link_libraries(blitzar_cpp_example PRIVATE blitzar)
    target_compile_features(blitzar_cpp_example PRIVATE cxx_std_20)
    blitzar_enable_warnings(blitzar_cpp_example)
endif()
