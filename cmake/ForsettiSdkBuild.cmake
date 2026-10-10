# Copyright (c) 2026 James Daley.
# SPDX-License-Identifier: Apache-2.0

# The installed static SDK is a deliberately bounded MSVC ABI payload.
if(NOT WIN32 OR NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    message(FATAL_ERROR "The Forsetti Windows SDK must be built on Windows with Microsoft MSVC.")
endif()
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8 OR
   NOT CMAKE_CXX_COMPILER_ARCHITECTURE_ID STREQUAL "x64")
    message(FATAL_ERROR "The Forsetti Windows SDK currently supports MSVC x64 only. Configure with -A x64.")
endif()
if(NOT FORSETTI_BUILD_HOST_TEMPLATE)
    message(FATAL_ERROR "FORSETTI_INSTALL_SDK requires FORSETTI_BUILD_HOST_TEMPLATE: the SDK contains all three runtime products.")
endif()

set(FORSETTI_SDK_MSVC_RUNTIME "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
if(DEFINED CMAKE_MSVC_RUNTIME_LIBRARY AND
   NOT CMAKE_MSVC_RUNTIME_LIBRARY STREQUAL FORSETTI_SDK_MSVC_RUNTIME)
    message(FATAL_ERROR "Forsetti SDK builds require CMAKE_MSVC_RUNTIME_LIBRARY=${FORSETTI_SDK_MSVC_RUNTIME} (/MDd Debug, /MD Release).")
endif()
set(CMAKE_MSVC_RUNTIME_LIBRARY "${FORSETTI_SDK_MSVC_RUNTIME}")

# vcpkg.json's immutable builtin-baseline selects nlohmann-json 3.12.0#2.
set(FORSETTI_SDK_NLOHMANN_JSON_VERSION "3.12.0")
set(FORSETTI_SDK_NLOHMANN_JSON_PORT_VERSION 2)
set(FORSETTI_SDK_VCPKG_BASELINE "cb2981c4e03d421fa03b9bb5044cd1986180e7e4")
find_package(nlohmann_json ${FORSETTI_SDK_NLOHMANN_JSON_VERSION} EXACT CONFIG REQUIRED)

set(FORSETTI_SDK_WINDOWS_SDK_VERSION "${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}")
if(FORSETTI_SDK_WINDOWS_SDK_VERSION STREQUAL "")
    set(FORSETTI_SDK_WINDOWS_SDK_VERSION "${CMAKE_SYSTEM_VERSION}")
endif()

configure_file("${PROJECT_SOURCE_DIR}/cmake/ForsettiSdkCompatibility.h.in"
    "${PROJECT_BINARY_DIR}/ForsettiSdkCompatibility.h" @ONLY)

# Build provenance is consumed by the allowlisted SDK assembler, not installed.
configure_file("${PROJECT_SOURCE_DIR}/cmake/ForsettiSdkBuildMetadata.json.in"
    "${PROJECT_BINARY_DIR}/ForsettiSdkBuildMetadata.json" @ONLY)
