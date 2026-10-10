# Copyright (c) 2026 James Daley.
# SPDX-License-Identifier: Apache-2.0

include(CMakePackageConfigHelpers)

set(_forsetti_sdk_targets ForsettiCore ForsettiPlatform ForsettiHostTemplate)
install(CODE [[
    if(NOT CMAKE_INSTALL_CONFIG_NAME MATCHES "^(Debug|Release)$")
        message(FATAL_ERROR "Install the Forsetti SDK with --config Debug or --config Release. Other configurations are unsupported.")
    endif()
]] COMPONENT ForsettiSdk)
foreach(_target IN LISTS _forsetti_sdk_targets)
    # Do not export repository warning policy (/W4 /WX). These ABI checks alone
    # travel with the installed static libraries and also guard SDK builds.
    target_compile_definitions(${_target} PRIVATE
        "FORSETTI_SDK_ITERATOR_DEBUG_LEVEL=$<IF:$<CONFIG:Debug>,2,0>")
    target_compile_options(${_target} PRIVATE
        "/FI${PROJECT_BINARY_DIR}/ForsettiSdkCompatibility.h")
    target_compile_definitions(${_target} INTERFACE
        "$<INSTALL_INTERFACE:FORSETTI_SDK_ITERATOR_DEBUG_LEVEL=$<IF:$<CONFIG:Debug>,2,0>>")
    target_compile_options(${_target} INTERFACE
        "$<INSTALL_INTERFACE:/FI$<INSTALL_PREFIX>/lib/cmake/ForsettiFramework/ForsettiSdkCompatibility.h>")
endforeach()

install(TARGETS ${_forsetti_sdk_targets}
    EXPORT ForsettiFrameworkTargets
    ARCHIVE DESTINATION "lib/$<CONFIG>"
    CONFIGURATIONS Debug Release
    COMPONENT ForsettiSdk)
install(DIRECTORY
    "${PROJECT_SOURCE_DIR}/include/ForsettiCore"
    "${PROJECT_SOURCE_DIR}/include/ForsettiPlatform"
    "${PROJECT_SOURCE_DIR}/include/ForsettiHostTemplate"
    DESTINATION include
    COMPONENT ForsettiSdk
    FILES_MATCHING PATTERN "*.h")

set(_forsetti_sdk_cmake_dir "lib/cmake/ForsettiFramework")
configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/cmake/ForsettiFrameworkConfig.cmake.in"
    "${PROJECT_BINARY_DIR}/ForsettiFrameworkConfig.cmake"
    INSTALL_DESTINATION "${_forsetti_sdk_cmake_dir}")
write_basic_package_version_file(
    "${PROJECT_BINARY_DIR}/ForsettiFrameworkConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY ExactVersion)
install(EXPORT ForsettiFrameworkTargets
    FILE ForsettiFrameworkTargets.cmake
    NAMESPACE Forsetti::
    DESTINATION "${_forsetti_sdk_cmake_dir}"
    CONFIGURATIONS Debug Release
    COMPONENT ForsettiSdk)
install(FILES
    "${PROJECT_BINARY_DIR}/ForsettiFrameworkConfig.cmake"
    "${PROJECT_BINARY_DIR}/ForsettiFrameworkConfigVersion.cmake"
    "${PROJECT_BINARY_DIR}/ForsettiSdkCompatibility.h"
    DESTINATION "${_forsetti_sdk_cmake_dir}"
    COMPONENT ForsettiSdk)
