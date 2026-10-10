# Forsetti Framework for Windows
# Copyright (c) 2026 James Daley.
# SPDX-License-Identifier: Apache-2.0
cmake_minimum_required(VERSION 3.28)
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)
    project(TemplateUIModule LANGUAGES CXX)
endif()
find_package(ForsettiFramework CONFIG REQUIRED)
add_library(TemplateUIModule STATIC TemplateUIModule.cpp TemplateUIModule.h
    ForsettiManifests/TemplateUIModule.json)
set_source_files_properties(ForsettiManifests/TemplateUIModule.json PROPERTIES HEADER_FILE_ONLY TRUE)
target_compile_features(TemplateUIModule PUBLIC cxx_std_20)
target_include_directories(TemplateUIModule PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(TemplateUIModule PUBLIC Forsetti::ForsettiCore)
