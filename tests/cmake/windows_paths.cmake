function(gravity_apply_test_paths target_name)
    if(COMMAND gravity_apply_windows_paths)
        gravity_apply_windows_paths(${target_name})
        return()
    endif()
    if(NOT WIN32)
        return()
    endif()

    set(PF86 "$ENV{SystemDrive}/Program Files (x86)")

    set(_incs "")
    foreach(_p IN ITEMS
        "${GRAVITY_MSVC_INCLUDE_DIR}"
        "${GRAVITY_WINSDK_INCLUDE_UCRT}"
        "${GRAVITY_WINSDK_INCLUDE_UM}"
        "${GRAVITY_WINSDK_INCLUDE_SHARED}"
        "${GRAVITY_WINSDK_INCLUDE_WINRT}"
        "${GRAVITY_WINSDK_INCLUDE_CPPWINRT}")
        if(NOT "${_p}" STREQUAL "" AND EXISTS "${_p}")
            list(APPEND _incs "${_p}")
        endif()
    endforeach()
    if(NOT _incs)
        file(GLOB _msvc_roots "${PF86}/Microsoft Visual Studio/2022/BuildTools/VC/Tools/MSVC/*")
        if(_msvc_roots)
            list(SORT _msvc_roots COMPARE NATURAL ORDER DESCENDING)
            list(GET _msvc_roots 0 _msvc_root)
            if(EXISTS "${_msvc_root}/include")
                list(APPEND _incs "${_msvc_root}/include")
            endif()
        endif()
        file(GLOB _winsdk_include_versions "${PF86}/Windows Kits/10/Include/*")
        if(_winsdk_include_versions)
            list(SORT _winsdk_include_versions COMPARE NATURAL ORDER DESCENDING)
            list(GET _winsdk_include_versions 0 _winsdk_root)
            foreach(_sdk_inc IN ITEMS ucrt um shared winrt cppwinrt)
                if(EXISTS "${_winsdk_root}/${_sdk_inc}")
                    list(APPEND _incs "${_winsdk_root}/${_sdk_inc}")
                endif()
            endforeach()
        endif()
    endif()
    if(_incs)
        target_include_directories(${target_name} SYSTEM PRIVATE ${_incs})
    endif()

    set(_libs "")
    foreach(_p IN ITEMS
        "${GRAVITY_MSVC_LIB_DIR}"
        "${GRAVITY_WINSDK_UCRT_LIB_DIR}"
        "${GRAVITY_WINSDK_UM_LIB_DIR}")
        if(NOT "${_p}" STREQUAL "" AND EXISTS "${_p}")
            list(APPEND _libs "${_p}")
        endif()
    endforeach()
    if(NOT _libs)
        file(GLOB _msvc_roots "${PF86}/Microsoft Visual Studio/2022/BuildTools/VC/Tools/MSVC/*")
        if(_msvc_roots)
            list(SORT _msvc_roots COMPARE NATURAL ORDER DESCENDING)
            list(GET _msvc_roots 0 _msvc_root)
            if(EXISTS "${_msvc_root}/lib/x64")
                list(APPEND _libs "${_msvc_root}/lib/x64")
            endif()
        endif()
        file(GLOB _winsdk_lib_versions "${PF86}/Windows Kits/10/Lib/*")
        if(_winsdk_lib_versions)
            list(SORT _winsdk_lib_versions COMPARE NATURAL ORDER DESCENDING)
            list(GET _winsdk_lib_versions 0 _winsdk_root)
            foreach(_sdk_lib IN ITEMS ucrt um)
                if(EXISTS "${_winsdk_root}/${_sdk_lib}/x64")
                    list(APPEND _libs "${_winsdk_root}/${_sdk_lib}/x64")
                endif()
            endforeach()
        endif()
    endif()
    if(_libs)
        target_link_directories(${target_name} PRIVATE ${_libs})
    endif()
endfunction()

if(WIN32 AND NOT COMMAND gravity_apply_windows_paths)
    set(_gravity_win_includes "")
    foreach(_p IN ITEMS
        "${GRAVITY_MSVC_INCLUDE_DIR}"
        "${GRAVITY_WINSDK_INCLUDE_UCRT}"
        "${GRAVITY_WINSDK_INCLUDE_UM}"
        "${GRAVITY_WINSDK_INCLUDE_SHARED}"
        "${GRAVITY_WINSDK_INCLUDE_WINRT}"
        "${GRAVITY_WINSDK_INCLUDE_CPPWINRT}")
        if(NOT "${_p}" STREQUAL "" AND EXISTS "${_p}")
            list(APPEND _gravity_win_includes "${_p}")
        endif()
    endforeach()
    if(_gravity_win_includes)
        list(REMOVE_DUPLICATES _gravity_win_includes)
        include_directories(SYSTEM ${_gravity_win_includes})
    endif()
endif()
