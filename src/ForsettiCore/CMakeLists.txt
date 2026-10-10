# ForsettiCore — Pure C++20, no platform dependencies
# Copyright (c) 2026 James Daley.
# SPDX-License-Identifier: Apache-2.0

add_library(ForsettiCore STATIC
    SemVer.cpp
    DefaultModuleRoles.cpp
    ModuleRequirements.cpp
    ModuleModels.cpp
    ModuleRegistration.cpp
    ModuleRequirementValidator.cpp
    UIModels.cpp
    ForsettiLogger.cpp
    ForsettiEventBus.cpp
    ForsettiServiceContainer.cpp
    ForsettiContext.cpp
    ActivationStore.cpp
    CapabilityPolicy.cpp
    EntitlementProviders.cpp
    ManifestLoader.cpp
    CompatibilityChecker.cpp
    ModuleRegistry.cpp
    StaticModuleRegistry.cpp
    UISurfaceManager.cpp
    ModuleManager.cpp
    ForsettiRuntime.cpp
)

target_include_directories(ForsettiCore PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(ForsettiCore PUBLIC cxx_std_20)

target_link_libraries(ForsettiCore PUBLIC
    nlohmann_json::nlohmann_json
)

# ForsettiCore must NOT link to ForsettiPlatform, ForsettiHostTemplate, or example modules
# This is enforced by the architecture tests and CI guardrails
