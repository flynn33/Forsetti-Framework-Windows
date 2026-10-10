# Forsetti Framework for Windows
# Copyright (c) 2026 James Daley.
# SPDX-License-Identifier: Apache-2.0
cmake_minimum_required(VERSION 3.28)
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)
project(TemplateHost LANGUAGES CXX)
find_package(ForsettiFramework CONFIG REQUIRED)
add_subdirectory(Modules/TemplateAppModule)
add_subdirectory(Modules/TemplateUIModule)
add_subdirectory(Modules/TemplateServiceModule)
set(app_manifests
    Modules/TemplateAppModule/ForsettiManifests/TemplateAppModule.json
    Modules/TemplateUIModule/ForsettiManifests/TemplateUIModule.json
    Modules/TemplateServiceModule/ForsettiManifests/TemplateServiceModule.json)
add_executable(TemplateHost
    src/Main.cpp src/ActivationStore.cpp src/ActivationStore.h
    src/NativePresentation.cpp src/NativePresentation.h
    resources/application.manifest ${app_manifests})
set_source_files_properties(${app_manifests} resources/application.manifest PROPERTIES HEADER_FILE_ONLY TRUE)
target_compile_features(TemplateHost PRIVATE cxx_std_20)
target_compile_definitions(TemplateHost PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
target_link_libraries(TemplateHost PRIVATE
    Forsetti::ForsettiHostTemplate TemplateAppModule TemplateUIModule TemplateServiceModule user32)
if(MSVC)
    target_compile_options(TemplateHost PRIVATE /utf-8)
    target_link_options(TemplateHost PRIVATE "/MANIFESTINPUT:${CMAKE_CURRENT_SOURCE_DIR}/resources/application.manifest")
endif()
add_custom_command(TARGET TemplateHost POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:TemplateHost>/ForsettiManifests")
foreach(manifest IN LISTS app_manifests)
    add_custom_command(TARGET TemplateHost POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/${manifest}"
            "$<TARGET_FILE_DIR:TemplateHost>/ForsettiManifests")
endforeach()
enable_testing()
add_test(NAME generated-consumer-native-verification COMMAND TemplateHost --verify)
