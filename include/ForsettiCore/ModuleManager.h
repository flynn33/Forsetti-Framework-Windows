// Forsetti Framework for Windows
// Copyright (c) 2026 James Daley. All Rights Reserved.
// Proprietary and Confidential. Patent Pending.

#pragma once
#include "ForsettiCore/ForsettiProtocols.h"
#include "ForsettiCore/ModuleModels.h"
#include "ForsettiCore/UIModels.h"
#include "ForsettiCore/ForsettiContext.h"
#include "ForsettiCore/ModuleRegistry.h"
#include "ForsettiCore/ActivationStore.h"
#include "ForsettiCore/CapabilityPolicy.h"
#include "ForsettiCore/CompatibilityChecker.h"
#include "ForsettiCore/UISurfaceManager.h"
#include "ForsettiCore/ModuleRegistration.h"
#include "ForsettiCore/ModuleRequirementValidator.h"
#include "ForsettiCore/DefaultModuleRoles.h"
#include <string>
#include <set>
#include <optional>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <vector>
#include <stdexcept>

namespace Forsetti {

// ---------------------------------------------------------------------------
// Error types
// ---------------------------------------------------------------------------

enum class ModuleManagerError {
    ModuleNotFound,
    IncompatibleModule,
    EntitlementRequired,
    AlreadyActive,
    NotActive,
    ModuleIdentityMismatch,
    ModuleTypeMismatch,
    ModuleVersionMismatch,
    ModuleManifestMismatch,
    CapabilityDenied,
    RegistrationMissing,
    RegistrationUnconfirmed,
    RegistrationMismatch,
    RequirementValidationFailed
};

class ModuleManagerException final : public std::runtime_error {
public:
    explicit ModuleManagerException(ModuleManagerError error, const std::string& message = "");

    [[nodiscard]] ModuleManagerError error() const noexcept;

private:
    ModuleManagerError error_;
};

struct ActivationRestoreFailure final {
    std::string moduleID;
    std::string message;

    bool operator==(const ActivationRestoreFailure&) const = default;
};

struct ActivationRestoreResult final {
    std::vector<std::string> restoredModuleIDs;
    std::vector<ActivationRestoreFailure> failures;

    [[nodiscard]] bool succeeded() const noexcept {
        return failures.empty();
    }

    bool operator==(const ActivationRestoreResult&) const = default;
};

// ---------------------------------------------------------------------------
// ModuleManager — core activation logic
// ---------------------------------------------------------------------------

class ModuleManager final {
public:
    ModuleManager(
        ModuleRegistry registry,
        std::shared_ptr<CompatibilityChecker> checker,
        std::shared_ptr<IEntitlementProvider> entitlementProvider,
        std::shared_ptr<IActivationStore> store,
        std::shared_ptr<UISurfaceManager> surfaceManager,
        std::shared_ptr<ForsettiContext> context,
        std::shared_ptr<ModuleRegistrationService> registrationService = nullptr,
        std::shared_ptr<ModuleRequirementValidator> requirementValidator = nullptr
    );

    // Discovery
    void discoverManifests(const std::string& manifestDirectory);

    // Activation / Deactivation
    void activateModule(const std::string& moduleID);
    void deactivateModule(const std::string& moduleID);

    // Persisted state restoration
    [[nodiscard]] ActivationRestoreResult restorePersistedActivation();

    // Getters
    [[nodiscard]] const std::set<std::string>& enabledServiceModuleIDs() const;
    [[nodiscard]] const std::set<std::string>& enabledUIModuleIDs() const;
    [[nodiscard]] const std::optional<std::string>& activeUIModuleID() const;
    [[nodiscard]] const std::unordered_map<std::string, ModuleManifest>& manifestsByID() const;
    [[nodiscard]] std::vector<ModuleRegistrationRecord> registeredModules() const;
    [[nodiscard]] bool isModuleActive(const std::string& moduleID) const;
    [[nodiscard]] ActivationRestoreResult lastRestoreResult() const;

private:
    void activateModuleLocked(const std::string& moduleID, bool persistAfterActivation);

    void validateResolvedModule(const IForsettiModule& module, const ModuleManifest& manifest) const;
    void validateConfirmedRegistration(const ModuleManifest& manifest) const;
    void validateRuntimeRequirements(
        const ModuleManifest& manifest,
        const ForsettiContext& moduleContext) const;
    void validateRequiredDefaultRoles(const ModuleManifest& manifest) const;
    [[nodiscard]] std::shared_ptr<ForsettiContext> makeModuleContext(
        const ModuleManifest& manifest) const;

    // UI-specific activation
    void activateUIModule(
        const std::string& moduleID,
        const ModuleManifest& manifest,
        IForsettiUIModule* uiModule,
        ForsettiContext& moduleContext);

    // State persistence
    void persistState();

    // Surface contribution normalization after ownership validation
    [[nodiscard]] UIContributions sanitizedUIContributions(const UIContributions& original) const;
    void validateUIContributions(const ModuleManifest& manifest, const UIContributions& contributions) const;

    // Dependencies
    ModuleRegistry registry_;
    std::shared_ptr<CompatibilityChecker> checker_;
    std::shared_ptr<IEntitlementProvider> entitlementProvider_;
    std::shared_ptr<IActivationStore> store_;
    std::shared_ptr<UISurfaceManager> surfaceManager_;
    std::shared_ptr<ForsettiContext> context_;
    std::shared_ptr<ModuleRegistrationService> registrationService_;
    std::shared_ptr<ModuleRequirementValidator> requirementValidator_;

    // Module state
    std::unordered_map<std::string, ModuleManifest> manifestsByID_;
    std::unordered_map<std::string, std::unique_ptr<IForsettiModule>> loadedModules_;
    std::unordered_map<std::string, std::shared_ptr<ForsettiContext>> moduleContexts_;
    std::set<std::string> enabledServiceModuleIDs_;
    std::set<std::string> enabledUIModuleIDs_;
    std::optional<std::string> activeUIModuleID_;
    ActivationRestoreResult lastRestoreResult_;

    mutable std::mutex mutex_;
};

} // namespace Forsetti
