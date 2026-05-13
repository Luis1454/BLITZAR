# @file cmake/qt_runtime_deploy.cmake
# @author Luis1454
# @project BLITZAR
# @brief CMake build orchestration for BLITZAR targets and tooling.

set(BLITZAR_DEPLOY_QT_RUNTIME ON CACHE BOOL "Deploy Qt runtime plugins next to built Qt client artifacts on Windows")

function(BLITZAR_find_windeployqt out_var)
    if(DEFINED BLITZAR_WINDEPLOYQT_EXECUTABLE AND EXISTS "${BLITZAR_WINDEPLOYQT_EXECUTABLE}")
        set(${out_var} "${BLITZAR_WINDEPLOYQT_EXECUTABLE}" PARENT_SCOPE)
        return()
    endif()

    set(_hints)
    if(DEFINED BLITZAR_QT_ROOT_DIR AND EXISTS "${BLITZAR_QT_ROOT_DIR}/bin")
        list(APPEND _hints "${BLITZAR_QT_ROOT_DIR}/bin")
    endif()
    if(TARGET Qt6::qmake)
        get_target_property(_qt_qmake_location Qt6::qmake IMPORTED_LOCATION)
        if(_qt_qmake_location)
            get_filename_component(_qt_bin_dir "${_qt_qmake_location}" DIRECTORY)
            list(APPEND _hints "${_qt_bin_dir}")
        endif()
    endif()
    if(DEFINED Qt6_DIR AND EXISTS "${Qt6_DIR}")
        get_filename_component(_qt_cmake_dir "${Qt6_DIR}" DIRECTORY)
        get_filename_component(_qt_lib_dir "${_qt_cmake_dir}" DIRECTORY)
        get_filename_component(_qt_root_dir "${_qt_lib_dir}" DIRECTORY)
        list(APPEND _hints "${_qt_root_dir}/bin")
    endif()

    find_program(_BLITZAR_windeployqt NAMES windeployqt HINTS ${_hints})
    if(_BLITZAR_windeployqt)
        set(BLITZAR_WINDEPLOYQT_EXECUTABLE "${_BLITZAR_windeployqt}" CACHE FILEPATH "Qt windeployqt executable" FORCE)
    endif()
    set(${out_var} "${_BLITZAR_windeployqt}" PARENT_SCOPE)
endfunction()

function(BLITZAR_configure_qt_runtime_deploy target_name)
    if(NOT WIN32)
        return()
    endif()
    if(NOT BLITZAR_DEPLOY_QT_RUNTIME)
        message(STATUS "BLITZAR_DEPLOY_QT_RUNTIME=OFF: skipping Qt runtime deployment for ${target_name}")
        return()
    endif()

    BLITZAR_find_windeployqt(_BLITZAR_windeployqt)
    if(NOT _BLITZAR_windeployqt)
        message(WARNING "windeployqt not found; Qt runtime deployment is disabled for ${target_name}")
        return()
    endif()

    add_custom_command(
        TARGET ${target_name} POST_BUILD
        COMMAND "${_BLITZAR_windeployqt}"
            --dir "$<TARGET_FILE_DIR:${target_name}>"
            --compiler-runtime
            --no-translations
            "$<TARGET_FILE:${target_name}>"
        VERBATIM
    )
endfunction()
