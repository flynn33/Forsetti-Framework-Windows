# ForsettiExampleServiceModule
# Copyright (c) 2026 James Daley. All Rights Reserved.

add_library(ForsettiExampleServiceModule STATIC
    ExampleServiceModule.cpp
    ExampleServiceModuleRegistry.cpp
)

target_include_directories(ForsettiExampleServiceModule PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ForsettiExampleServiceModule PUBLIC
    ForsettiCore
)

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/Resources/ForsettiManifests/ExampleServiceModule.json"
     DESTINATION "${CMAKE_BINARY_DIR}/ForsettiManifests")
