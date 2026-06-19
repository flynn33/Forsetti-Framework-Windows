# ForsettiHostTemplate - sealed host composition layer
# Copyright (c) 2026 James Daley. All Rights Reserved.

add_library(ForsettiHostTemplate STATIC
    ForsettiHostState.cpp
    ForsettiHostController.cpp
    ForsettiHostBootstrap.cpp
    ForsettiHostOverlayRouter.cpp
    ForsettiSurfaceStateAdapter.cpp
)

target_include_directories(ForsettiHostTemplate PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(ForsettiHostTemplate PUBLIC
    ForsettiCore
    ForsettiPlatform
)

# ForsettiHostTemplate must not depend on example or downstream application targets.
