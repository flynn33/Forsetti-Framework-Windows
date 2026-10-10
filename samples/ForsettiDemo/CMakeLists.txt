# ForsettiDemo - downstream demonstration application
# Copyright (c) 2026 James Daley.
# SPDX-License-Identifier: Apache-2.0

add_executable(ForsettiDemo
    main.cpp
)

target_include_directories(ForsettiDemo PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(ForsettiDemo PRIVATE
    ForsettiHostTemplate
    ForsettiExampleServiceModule
    ForsettiExampleUIModule
    ForsettiExampleAppModule
)
