# @file cmake/qt_paths.cmake
# @brief Windows Qt6 lookup helpers for local desktop builds.

function(BLITZAR_register_qt6_widgets_from_root qt_root)
    if(TARGET Qt6::Widgets)
        return()
    endif()

    if(NOT EXISTS "${qt_root}/lib/cmake/Qt6/Qt6Targets.cmake")
        return()
    endif()
    if(NOT EXISTS "${qt_root}/lib/cmake/Qt6Core/Qt6CoreTargets.cmake")
        return()
    endif()
    if(NOT EXISTS "${qt_root}/lib/cmake/Qt6Gui/Qt6GuiTargets.cmake")
        return()
    endif()
    if(NOT EXISTS "${qt_root}/lib/cmake/Qt6Widgets/Qt6WidgetsTargets.cmake")
        return()
    endif()

    if(NOT TARGET Threads::Threads)
        add_library(Threads::Threads INTERFACE IMPORTED)
    endif()

    if(NOT TARGET WrapAtomic::WrapAtomic)
        add_library(WrapAtomic::WrapAtomic INTERFACE IMPORTED)
    endif()

    include("${qt_root}/lib/cmake/Qt6/Qt6Targets.cmake")
    include("${qt_root}/lib/cmake/Qt6Core/Qt6CoreTargets.cmake")
    include("${qt_root}/lib/cmake/Qt6Gui/Qt6GuiTargets.cmake")
    include("${qt_root}/lib/cmake/Qt6Widgets/Qt6WidgetsTargets.cmake")

    if(TARGET Qt6::Platform)
        set_property(TARGET Qt6::Platform PROPERTY INTERFACE_COMPILE_FEATURES "")
    endif()

    set(Qt6_DIR "${qt_root}/lib/cmake/Qt6" CACHE PATH "Qt6 package directory" FORCE)
    set(BLITZAR_QT_ROOT_DIR "${qt_root}" CACHE PATH "Qt6 root directory" FORCE)
endfunction()

function(BLITZAR_find_qt6_widgets)
    if(TARGET Qt6::Widgets)
        return()
    endif()

    if(NOT WIN32)
        find_package(Qt6 QUIET COMPONENTS Widgets)
        return()
    endif()

    set(_qt_candidate_roots "")
    if(DEFINED ENV{QT_DIR})
        if(EXISTS "$ENV{QT_DIR}/lib/cmake/Qt6/Qt6Targets.cmake")
            list(APPEND _qt_candidate_roots "$ENV{QT_DIR}")
        endif()
        file(GLOB _qt_env_roots "$ENV{QT_DIR}/*/msvc*_64")
        list(APPEND _qt_candidate_roots ${_qt_env_roots})
    endif()

    file(GLOB _qt_system_roots "$ENV{SystemDrive}/Qt/*/msvc*_64")
    list(APPEND _qt_candidate_roots ${_qt_system_roots})
    list(REMOVE_DUPLICATES _qt_candidate_roots)
    list(SORT _qt_candidate_roots COMPARE NATURAL ORDER DESCENDING)

    foreach(_qt_root IN LISTS _qt_candidate_roots)
        BLITZAR_register_qt6_widgets_from_root("${_qt_root}")
        if(TARGET Qt6::Widgets)
            return()
        endif()
    endforeach()

    find_package(Qt6 QUIET COMPONENTS Widgets)
endfunction()
