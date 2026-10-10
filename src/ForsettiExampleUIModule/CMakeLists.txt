# ForsettiExampleUIModule
# Copyright (c) 2026 James Daley.
# SPDX-License-Identifier: Apache-2.0

add_library(ForsettiExampleUIModule STATIC
    ExampleUIModule.cpp
    ExampleUIModuleRegistry.cpp
)

target_include_directories(ForsettiExampleUIModule PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ForsettiExampleUIModule PUBLIC
    ForsettiCore
)

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/Resources/ForsettiManifests/ExampleUIModule.json"
     DESTINATION "${CMAKE_BINARY_DIR}/ForsettiManifests")
