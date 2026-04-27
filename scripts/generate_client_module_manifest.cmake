# File: scripts/generate_client_module_manifest.cmake
# Purpose: Automation script for BLITZAR build, release, or operations tasks.

cmake_minimum_required(VERSION 3.24)

if(NOT DEFINED MODULE_FILE OR MODULE_FILE STREQUAL "")
    message(FATAL_ERROR "MODULE_FILE is required")
endif()
if(NOT DEFINED MODULE_ID OR MODULE_ID STREQUAL "")
    message(FATAL_ERROR "MODULE_ID is required")
endif()
if(NOT DEFINED MODULE_NAME OR MODULE_NAME STREQUAL "")
    message(FATAL_ERROR "MODULE_NAME is required")
endif()
if(NOT DEFINED API_VERSION OR API_VERSION STREQUAL "")
    message(FATAL_ERROR "API_VERSION is required")
endif()
if(NOT DEFINED PRODUCT_NAME OR PRODUCT_NAME STREQUAL "")
    message(FATAL_ERROR "PRODUCT_NAME is required")
endif()
if(NOT DEFINED PRODUCT_VERSION OR PRODUCT_VERSION STREQUAL "")
    message(FATAL_ERROR "PRODUCT_VERSION is required")
endif()

get_filename_component(_module_file_name "${MODULE_FILE}" NAME)
file(SHA256 "${MODULE_FILE}" _module_sha256)
set(_manifest_path "${MODULE_FILE}.manifest")

file(WRITE "${_manifest_path}"
    "format_version=1\n"
    "module_id=${MODULE_ID}\n"
    "module_name=${MODULE_NAME}\n"
    "api_version=${API_VERSION}\n"
    "product_name=${PRODUCT_NAME}\n"
    "product_version=${PRODUCT_VERSION}\n"
    "library_file=${_module_file_name}\n"
    "sha256=${_module_sha256}\n")
