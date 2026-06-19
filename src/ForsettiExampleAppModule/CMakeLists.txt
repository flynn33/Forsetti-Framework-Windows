# ForsettiExampleAppModule
# Copyright (c) 2026 James Daley. All Rights Reserved.

add_library(ForsettiExampleAppModule STATIC
    ExampleAppModule.cpp
    ExampleAppModuleRegistry.cpp
)

target_include_directories(ForsettiExampleAppModule PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ForsettiExampleAppModule PUBLIC
    ForsettiCore
)

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/Resources/ForsettiManifests/ExampleAppModule.json"
     DESTINATION "${CMAKE_BINARY_DIR}/ForsettiManifests")
