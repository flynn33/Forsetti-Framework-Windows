# Forsetti Windows service module starter
# Copyright (c) 2026 James Daley. All Rights Reserved.

add_library(TemplateServiceModule STATIC
    TemplateServiceModule.cpp
)

target_include_directories(TemplateServiceModule PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(TemplateServiceModule PUBLIC
    ForsettiCore
)
