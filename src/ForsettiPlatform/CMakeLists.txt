# ForsettiPlatform — Windows service implementations
# Copyright (c) 2026 James Daley.
# SPDX-License-Identifier: Apache-2.0

add_library(ForsettiPlatform STATIC
    PlatformServices.cpp
    ModuleRegistrationPlatform.cpp
    WindowsViewFactoryRegistry.cpp
    EntitlementProviders.cpp
    DefaultPlatformServices.cpp
)

target_include_directories(ForsettiPlatform PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(ForsettiPlatform PUBLIC cxx_std_20)

target_link_libraries(ForsettiPlatform PUBLIC
    ForsettiCore
)

# Windows SDK libraries for platform services
target_link_libraries(ForsettiPlatform PRIVATE
    advapi32
    bcrypt
    crypt32
    winhttp
)

# ForsettiPlatform must NOT link to ForsettiHostTemplate or example modules
