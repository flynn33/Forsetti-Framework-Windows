# ForsettiHostTemplate - sealed host composition layer
# Copyright (c) 2026 James Daley.
# SPDX-License-Identifier: Apache-2.0

add_library(ForsettiHostTemplate STATIC
    ForsettiHostState.cpp
    ForsettiHostController.cpp
    ForsettiHostBootstrap.cpp
    ForsettiHostOverlayRouter.cpp
    ForsettiSurfaceStateAdapter.cpp
)

target_include_directories(ForsettiHostTemplate PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(ForsettiHostTemplate PUBLIC cxx_std_20)

target_link_libraries(ForsettiHostTemplate PUBLIC
    ForsettiCore
    ForsettiPlatform
)

# ForsettiHostTemplate must not depend on example or downstream application targets.
