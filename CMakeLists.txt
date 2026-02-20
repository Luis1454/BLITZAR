cmake_minimum_required(VERSION 3.24)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if(EXISTS "C:/vcpkg/scripts/buildsystems/vcpkg.cmake")
        set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE FILEPATH "vcpkg toolchain file")
    endif()
endif()

if(WIN32)
    set(VCPKG_APPLOCAL_DEPS OFF)
endif()

if(WIN32)
    file(GLOB _msvc_roots "C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Tools/MSVC/*")
    if(_msvc_roots)
        list(SORT _msvc_roots COMPARE NATURAL ORDER DESCENDING)
        list(GET _msvc_roots 0 _msvc_root)
        set(_msvc_host_bin "${_msvc_root}/bin/Hostx64/x64")
        set(GRAVITY_MSVC_LIB_DIR "${_msvc_root}/lib/x64")
        set(GRAVITY_MSVC_INCLUDE_DIR "${_msvc_root}/include")
        set(ENV{PATH} "${_msvc_host_bin};$ENV{PATH}")
        if(EXISTS "${_msvc_host_bin}/cl.exe" AND NOT DEFINED CMAKE_CUDA_HOST_COMPILER)
            set(CMAKE_CUDA_HOST_COMPILER "${_msvc_host_bin}/cl.exe" CACHE FILEPATH "MSVC host compiler for nvcc" FORCE)
        endif()
        if(EXISTS "${_msvc_root}/include")
            set(ENV{INCLUDE} "${_msvc_root}/include;$ENV{INCLUDE}")
        endif()
        if(EXISTS "${_msvc_root}/lib/x64")
            set(ENV{LIB} "${_msvc_root}/lib/x64;$ENV{LIB}")
            set(ENV{LIBPATH} "${_msvc_root}/lib/x64;$ENV{LIBPATH}")
        endif()
    endif()

    file(GLOB _winsdk_x64_bins "C:/Program Files (x86)/Windows Kits/10/bin/*/x64")
    if(_winsdk_x64_bins)
        list(SORT _winsdk_x64_bins COMPARE NATURAL ORDER DESCENDING)
        list(GET _winsdk_x64_bins 0 _winsdk_x64_bin)
        set(ENV{PATH} "${_winsdk_x64_bin};$ENV{PATH}")
        if(EXISTS "${_winsdk_x64_bin}/rc.exe" AND NOT DEFINED CMAKE_RC_COMPILER)
            set(CMAKE_RC_COMPILER "${_winsdk_x64_bin}/rc.exe" CACHE FILEPATH "Windows SDK resource compiler" FORCE)
        endif()
        if(EXISTS "${_winsdk_x64_bin}/mt.exe" AND NOT DEFINED CMAKE_MT)
            set(CMAKE_MT "${_winsdk_x64_bin}/mt.exe" CACHE FILEPATH "Windows SDK manifest tool" FORCE)
        endif()
    endif()

    file(GLOB _winsdk_include_versions "C:/Program Files (x86)/Windows Kits/10/Include/*")
    if(_winsdk_include_versions)
        list(SORT _winsdk_include_versions COMPARE NATURAL ORDER DESCENDING)
        list(GET _winsdk_include_versions 0 _winsdk_include_root)
        set(GRAVITY_WINSDK_INCLUDE_UCRT "${_winsdk_include_root}/ucrt")
        set(GRAVITY_WINSDK_INCLUDE_UM "${_winsdk_include_root}/um")
        set(GRAVITY_WINSDK_INCLUDE_SHARED "${_winsdk_include_root}/shared")
        set(GRAVITY_WINSDK_INCLUDE_WINRT "${_winsdk_include_root}/winrt")
        set(GRAVITY_WINSDK_INCLUDE_CPPWINRT "${_winsdk_include_root}/cppwinrt")
        set(ENV{INCLUDE} "${_winsdk_include_root}/ucrt;${_winsdk_include_root}/um;${_winsdk_include_root}/shared;${_winsdk_include_root}/winrt;${_winsdk_include_root}/cppwinrt;$ENV{INCLUDE}")
    endif()

    file(GLOB _winsdk_lib_versions "C:/Program Files (x86)/Windows Kits/10/Lib/*")
    if(_winsdk_lib_versions)
        list(SORT _winsdk_lib_versions COMPARE NATURAL ORDER DESCENDING)
        list(GET _winsdk_lib_versions 0 _winsdk_lib_root)
        set(GRAVITY_WINSDK_UCRT_LIB_DIR "${_winsdk_lib_root}/ucrt/x64")
        set(GRAVITY_WINSDK_UM_LIB_DIR "${_winsdk_lib_root}/um/x64")
        set(ENV{LIB} "${_winsdk_lib_root}/ucrt/x64;${_winsdk_lib_root}/um/x64;$ENV{LIB}")
        set(ENV{LIBPATH} "${_winsdk_lib_root}/ucrt/x64;${_winsdk_lib_root}/um/x64;$ENV{LIBPATH}")
    endif()
endif()

project(CUDA_GRAVITY_SIMULATION LANGUAGES CUDA CXX)

set(APP_NAME myApp)
set(SFML_FRONTEND_NAME myAppFrontend)
set(QT_FRONTEND_NAME myAppQt)

option(GRAVITY_BUILD_SFML_FRONTEND "Build SFML frontend" ON)
option(GRAVITY_BUILD_QT_FRONTEND "Build Qt frontend" ON)
option(GRAVITY_BUILD_TESTS "Build deterministic physics regression tests" ON)
option(GRAVITY_PROFILE_LOGS "Print backend timings each update" OFF)

set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
    set(CMAKE_CUDA_ARCHITECTURES native)
endif()

find_package(CUDAToolkit REQUIRED)

set(GRAVITY_BACKEND_SOURCES
    src/cpp/sim/SimulationArgs.cpp
    src/cuda/sim/SimulationBackend.cu
    src/cpp/sim/SimulationConfig.cpp
    src/cuda/core/ParticleSystem.cu
)

function(configure_gravity_target target_name)
    target_include_directories(${target_name}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/fragments
    )

    if(WIN32)
        target_include_directories(${target_name}
            SYSTEM PRIVATE
                "${GRAVITY_MSVC_INCLUDE_DIR}"
                "${GRAVITY_WINSDK_INCLUDE_UCRT}"
                "${GRAVITY_WINSDK_INCLUDE_UM}"
                "${GRAVITY_WINSDK_INCLUDE_SHARED}"
                "${GRAVITY_WINSDK_INCLUDE_WINRT}"
                "${GRAVITY_WINSDK_INCLUDE_CPPWINRT}"
        )
    endif()

    set_target_properties(${target_name}
        PROPERTIES
            CUDA_SEPARABLE_COMPILATION ON
    )

    target_link_libraries(${target_name}
        PRIVATE
            CUDA::cudart
    )

    if(GRAVITY_PROFILE_LOGS)
        target_compile_definitions(${target_name}
            PRIVATE
                GRAVITY_PROFILE_LOGS=1
        )
    endif()

    if(MSVC)
        target_compile_options(${target_name}
            PRIVATE
                $<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=/Zc:__cplusplus>
        )
    endif()

    if(WIN32)
        target_link_directories(${target_name}
            PRIVATE
                "${GRAVITY_MSVC_LIB_DIR}"
                "${GRAVITY_WINSDK_UCRT_LIB_DIR}"
                "${GRAVITY_WINSDK_UM_LIB_DIR}"
        )
    endif()
endfunction()

function(copy_vcpkg_runtime target_name)
    if(WIN32)
        if(EXISTS "C:/vcpkg/installed/x64-windows/bin")
            add_custom_command(TARGET ${target_name}
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "C:/vcpkg/installed/x64-windows/bin"
                    "$<TARGET_FILE_DIR:${target_name}>"
            )
        endif()
        if(EXISTS "C:/vcpkg/installed/x64-windows/debug/bin")
            add_custom_command(TARGET ${target_name}
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "C:/vcpkg/installed/x64-windows/debug/bin"
                    "$<TARGET_FILE_DIR:${target_name}>"
            )
        endif()
    endif()
endfunction()

function(copy_runtime_directory target_name runtime_dir)
    if(WIN32 AND EXISTS "${runtime_dir}")
        add_custom_command(TARGET ${target_name}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${runtime_dir}"
                "$<TARGET_FILE_DIR:${target_name}>"
        )
    endif()
endfunction()

function(copy_runtime_subdirectory target_name runtime_dir subdir_name)
    if(WIN32 AND EXISTS "${runtime_dir}")
        add_custom_command(TARGET ${target_name}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:${target_name}>/${subdir_name}"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${runtime_dir}"
                "$<TARGET_FILE_DIR:${target_name}>/${subdir_name}"
        )
    endif()
endfunction()

add_executable(${APP_NAME}
    src/cuda/app/main_headless.cu
    ${GRAVITY_BACKEND_SOURCES}
)
configure_gravity_target(${APP_NAME})

if(GRAVITY_BUILD_SFML_FRONTEND)
    find_package(SFML CONFIG QUIET COMPONENTS Graphics Window System)
    if(NOT SFML_FOUND)
        find_package(SFML 2.5 QUIET COMPONENTS graphics window system)
    endif()

    if(SFML_FOUND)
        add_executable(${SFML_FRONTEND_NAME}
            src/cpp/app/main_sfml.cpp
            ${GRAVITY_BACKEND_SOURCES}
        )
        configure_gravity_target(${SFML_FRONTEND_NAME})

        if(TARGET SFML::Graphics)
            target_link_libraries(${SFML_FRONTEND_NAME}
                PRIVATE
                    SFML::Graphics
                    SFML::Window
                    SFML::System
            )
        else()
            target_link_libraries(${SFML_FRONTEND_NAME}
                PRIVATE
                    ${SFML_LIBRARIES}
            )
        endif()

        copy_vcpkg_runtime(${SFML_FRONTEND_NAME})
    else()
        message(STATUS "SFML not found. Frontend target '${SFML_FRONTEND_NAME}' is disabled.")
    endif()
endif()

if(GRAVITY_BUILD_QT_FRONTEND)
    find_package(Qt6 QUIET COMPONENTS Widgets)
    if(NOT Qt6_FOUND AND WIN32)
        file(GLOB _qt_local_roots "C:/Qt/*/msvc*_64")
        list(SORT _qt_local_roots COMPARE NATURAL ORDER DESCENDING)
        foreach(_qt_root IN LISTS _qt_local_roots)
            if(EXISTS "${_qt_root}/lib/cmake/Qt6/Qt6Config.cmake")
                list(PREPEND CMAKE_PREFIX_PATH "${_qt_root}")
                break()
            endif()
        endforeach()
        find_package(Qt6 QUIET COMPONENTS Widgets)
    endif()

    if(Qt6_FOUND)
        add_executable(${QT_FRONTEND_NAME}
            src/cpp/app/main_qt.cpp
            src/cpp/ui/EnergyGraphWidget.cpp
            src/cpp/ui/MainWindow.cpp
            src/cpp/ui/MultiViewWidget.cpp
            src/cpp/ui/ParticleView.cpp
            src/cpp/ui/QtViewMath.cpp
            ${GRAVITY_BACKEND_SOURCES}
        )
        configure_gravity_target(${QT_FRONTEND_NAME})
        target_link_libraries(${QT_FRONTEND_NAME}
            PRIVATE
                Qt6::Widgets
        )

        if(TARGET Qt6::Core)
            get_target_property(_qt_core_release Qt6::Core IMPORTED_LOCATION_RELEASE)
            if(NOT _qt_core_release)
                get_target_property(_qt_core_release Qt6::Core IMPORTED_LOCATION)
            endif()
            if(_qt_core_release)
                get_filename_component(_qt_bin_dir "${_qt_core_release}" DIRECTORY)
                get_filename_component(_qt_root_dir "${_qt_bin_dir}" DIRECTORY)
                copy_runtime_directory(${QT_FRONTEND_NAME} "${_qt_bin_dir}")
                copy_runtime_subdirectory(${QT_FRONTEND_NAME} "${_qt_root_dir}/plugins/platforms" "platforms")
                copy_runtime_subdirectory(${QT_FRONTEND_NAME} "${_qt_root_dir}/plugins/styles" "styles")

                if(EXISTS "${_qt_bin_dir}/windeployqt.exe")
                    add_custom_command(TARGET ${QT_FRONTEND_NAME}
                        POST_BUILD
                        COMMAND "${_qt_bin_dir}/windeployqt.exe"
                            --no-compiler-runtime
                            --no-translations
                            "$<TARGET_FILE:${QT_FRONTEND_NAME}>"
                    )
                endif()
            endif()
        endif()
    else()
        message(STATUS "Qt6 not found. Frontend target '${QT_FRONTEND_NAME}' is disabled.")
    endif()
endif()

if(GRAVITY_BUILD_TESTS)
    enable_testing()

    find_package(GTest CONFIG QUIET)
    if(NOT TARGET GTest::gtest_main)
        include(FetchContent)
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
        FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
        )
        FetchContent_MakeAvailable(googletest)

        if(WIN32)
            set(_gravity_windows_system_includes "")
            foreach(_dir IN ITEMS
                "${GRAVITY_MSVC_INCLUDE_DIR}"
                "${GRAVITY_WINSDK_INCLUDE_UCRT}"
                "${GRAVITY_WINSDK_INCLUDE_UM}"
                "${GRAVITY_WINSDK_INCLUDE_SHARED}"
                "${GRAVITY_WINSDK_INCLUDE_WINRT}"
                "${GRAVITY_WINSDK_INCLUDE_CPPWINRT}")
                if(EXISTS "${_dir}")
                    list(APPEND _gravity_windows_system_includes "${_dir}")
                endif()
            endforeach()

            foreach(_gtest_target IN ITEMS gtest gtest_main gmock gmock_main)
                if(TARGET ${_gtest_target} AND _gravity_windows_system_includes)
                    target_include_directories(${_gtest_target}
                        SYSTEM PRIVATE
                            ${_gravity_windows_system_includes}
                    )
                endif()
            endforeach()
        endif()
    endif()

    add_executable(gravityPhysicsGTests
        tests/physics_regression.cpp
        ${GRAVITY_BACKEND_SOURCES}
    )
    configure_gravity_target(gravityPhysicsGTests)
    target_compile_definitions(gravityPhysicsGTests
        PRIVATE
            GRAVITY_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    )
    target_link_libraries(gravityPhysicsGTests
        PRIVATE
            GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(gravityPhysicsGTests)
endif()
