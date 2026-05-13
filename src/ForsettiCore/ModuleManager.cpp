// Forsetti Framework for Windows
// Copyright (c) 2026 James Daley. All Rights Reserved.
// Proprietary and Confidential. Patent Pending.

#include "ForsettiCore/ModuleManager.h"
#include "ForsettiCore/ManifestLoader.h"
#include <filesystem>
#include <algorithm>

namespace Forsetti {

// ---------------------------------------------------------------------------
// ModuleManagerException
// ---------------------------------------------------------------------------

ModuleManagerException::ModuleManagerException(ModuleManagerError error, const std::string& message)
    : std::runtime_error(message.empty() ? "ModuleManagerError" : message)
    , error_(error)
{
}

ModuleManagerError ModuleManagerException::error() const noexcept
{
    return error_;
}

// ---------------------------------------------------------------------------
// ModuleManager — construction
// ---------------------------------------------------------------------------

ModuleManager::ModuleManager(
    ModuleRegistry registry,
    std::shared_ptr<CompatibilityChecker> checker,
    std::shared_ptr<IEntitlementProvider> entitlementProvider,
    std::shared_ptr<IActivationStore> store,
    std::shared_ptr<UISurfaceManager> surfaceManager,
    std::shared_ptr<ForsettiContext> context)
    : registry_(std::move(registry))
    , checker_(std::move(checker))
    , entitlementProvider_(std::move(entitlementProvider))
    , store_(std::move(store))
    , surfaceManager_(std::move(surfaceManager))
    , context_(std::move(context))
{
}

// ---------------------------------------------------------------------------
// Discovery
// ---------------------------------------------------------------------------

void ModuleManager::discoverManifests(const std::string& manifestDirectory)
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto manifests = ManifestLoader::loadManifests(manifestDirectory);
    for (auto& manifest : manifests) {
        manifestsByID_[manifest.moduleID] = std::move(manifest);
    }
}

// ---------------------------------------------------------------------------
// Activation
// ---------------------------------------------------------------------------

void ModuleManager::activateModule(const std::string& moduleID)
{
    std::lock_guard<std::mutex> lock(mutex_);
    activateModuleLocked(moduleID, true);
}

void ModuleManager::activateModuleLocked(const std::string& moduleID, bool persistAfterActivation)
{
    // 1. Find manifest
    auto manifestIt = manifestsByID_.find(moduleID);
    if (manifestIt == manifestsByID_.end()) {
        throw ModuleManagerException(
            ModuleManagerError::ModuleNotFound,
            "Module not found: " + moduleID);
    }
    const auto& manifest = manifestIt->second;

    // 2. Compatibility check
    auto report = checker_->checkCompatibility(manifest);
    if (!report.isCompatible()) {
        throw ModuleManagerException(
            ModuleManagerError::IncompatibleModule,
            "Module is incompatible: " + moduleID);
    }

    // 3. Entitlement check — unlocked by moduleID or iapProductID
    bool unlocked = entitlementProvider_->isUnlocked(moduleID);
    if (!unlocked && manifest.iapProductID.has_value()) {
        unlocked = entitlementProvider_->isUnlocked(manifest.iapProductID.value());
    }
    if (!unlocked) {
        throw ModuleManagerException(
            ModuleManagerError::EntitlementRequired,
            "Entitlement required for module: " + moduleID);
    }

    // 4. Already active?
    if (loadedModules_.find(moduleID) != loadedModules_.end()) {
        throw ModuleManagerException(
            ModuleManagerError::AlreadyActive,
            "Module already active: " + moduleID);
    }

    // 5. Create and validate module via registry
    auto module = registry_.makeModule(manifest.entryPoint);
    if (!module) {
        throw ModuleManagerException(
            ModuleManagerError::ModuleNotFound,
            "Registry has no factory for entry point: " + manifest.entryPoint);
    }
    validateResolvedModule(*module, manifest);
    auto moduleContext = makeModuleContext(manifest);

    // 6. Route by type
    try {
        if (manifest.moduleType == ModuleType::Service) {
            module->start(*moduleContext);
            enabledServiceModuleIDs_.insert(moduleID);
        } else {
            auto* uiModule = dynamic_cast<IForsettiUIModule*>(module.get());
            if (!uiModule) {
                throw ModuleManagerException(
                    ModuleManagerError::ModuleTypeMismatch,
                    "Module does not satisfy UI activation contract: " + moduleID);
            }

            if (manifest.moduleType == ModuleType::App &&
                dynamic_cast<IForsettiAppModule*>(module.get()) == nullptr) {
                throw ModuleManagerException(
                    ModuleManagerError::ModuleTypeMismatch,
                    "App module does not satisfy app activation contract: " + moduleID);
            }

            activateUIModule(moduleID, uiModule, *moduleContext);
        }
    } catch (...) {
        enabledServiceModuleIDs_.erase(moduleID);
        enabledUIModuleIDs_.erase(moduleID);
        if (activeUIModuleID_.has_value() && activeUIModuleID_.value() == moduleID) {
            activeUIModuleID_.reset();
        }
        throw;
    }

    // 7. Store loaded module
    moduleContexts_[moduleID] = std::move(moduleContext);
    loadedModules_[moduleID] = std::move(module);

    // 8. Persist state
    if (persistAfterActivation) {
        persistState();
    }
}

// ---------------------------------------------------------------------------
// UI Module Activation (private)
// ---------------------------------------------------------------------------

void ModuleManager::activateUIModule(
    const std::string& moduleID,
    IForsettiUIModule* uiModule,
    ForsettiContext& moduleContext)
{
    // 1. Prepare and start the incoming module before mutating active UI state.
    auto contributions = uiModule->uiContributions();
    auto sanitized = sanitizedUIContributions(contributions);

    uiModule->start(moduleContext);

    // 2. If there is already an active UI module, deactivate it first.
    if (activeUIModuleID_.has_value()) {
        const auto& previousID = activeUIModuleID_.value();
        auto prevIt = loadedModules_.find(previousID);
        if (prevIt != loadedModules_.end()) {
            auto contextIt = moduleContexts_.find(previousID);
            auto& previousContext =
                contextIt != moduleContexts_.end() ? *contextIt->second : *context_;
            prevIt->second->stop(previousContext);
            surfaceManager_->removeModuleContributions(previousID);
        }
        enabledUIModuleIDs_.erase(previousID);
        // Note: we don't remove from loadedModules_ here — full deactivation
        // is only done via deactivateModule(). We just switch the active UI.
        activeUIModuleID_.reset();
    }

    // 3. Add to surface manager
    surfaceManager_->addModuleContributions(moduleID, sanitized);

    // 4. Rebuild surface state
    surfaceManager_->rebuildSurfaceState();

    // 5. Track as enabled UI module
    enabledUIModuleIDs_.insert(moduleID);

    // 6. Set as active UI module
    activeUIModuleID_ = moduleID;
}

// ---------------------------------------------------------------------------
// Deactivation
// ---------------------------------------------------------------------------

void ModuleManager::deactivateModule(const std::string& moduleID)
{
    std::lock_guard<std::mutex> lock(mutex_);

    // 1. Find loaded module
    auto it = loadedModules_.find(moduleID);
    if (it == loadedModules_.end()) {
        throw ModuleManagerException(
            ModuleManagerError::NotActive,
            "Module is not active: " + moduleID);
    }

    // 2. Stop the module
    auto contextIt = moduleContexts_.find(moduleID);
    auto& moduleContext =
        contextIt != moduleContexts_.end() ? *contextIt->second : *context_;
    it->second->stop(moduleContext);

    // 3. Remove from the appropriate enabled set
    enabledServiceModuleIDs_.erase(moduleID);
    enabledUIModuleIDs_.erase(moduleID);

    // 4. If it was the active UI module, clean up surface contributions
    if (activeUIModuleID_.has_value() && activeUIModuleID_.value() == moduleID) {
        surfaceManager_->removeModuleContributions(moduleID);
        surfaceManager_->rebuildSurfaceState();
        activeUIModuleID_.reset();
    }

    // 5. Remove from loaded modules
    loadedModules_.erase(it);
    moduleContexts_.erase(moduleID);

    // 6. Persist state
    persistState();
}

// ---------------------------------------------------------------------------
// Persisted Activation Restoration
// ---------------------------------------------------------------------------

ActivationRestoreResult ModuleManager::restorePersistedActivation()
{
    std::lock_guard<std::mutex> lock(mutex_);

    ActivationRestoreResult result;
    auto state = store_->loadState();
    const bool hadPersistedActivation =
        !state.enabledServiceModuleIDs.empty() ||
        !state.enabledUIModuleIDs.empty() ||
        state.selectedUIModuleID.has_value();

    auto restoreOne = [this, &result](const std::string& moduleID) {
        try {
            if (!isModuleActive(moduleID)) {
                activateModuleLocked(moduleID, false);
            }
            result.restoredModuleIDs.push_back(moduleID);
        } catch (const std::exception& ex) {
            result.failures.push_back({moduleID, ex.what()});
            if (context_ && context_->logger()) {
                context_->logger()->log(
                    LogLevel::Warning,
                    "Failed to restore module: " + moduleID + ": " + ex.what(),
                    moduleID);
            }
        } catch (...) {
            result.failures.push_back({moduleID, "Unknown restore failure"});
            if (context_ && context_->logger()) {
                context_->logger()->log(
                    LogLevel::Warning,
                    "Failed to restore module: " + moduleID + ": unknown failure",
                    moduleID);
            }
        }
    };

    // Restore service modules first.
    for (const auto& moduleID : state.enabledServiceModuleIDs) {
        restoreOne(moduleID);
    }

    // Restore the selected UI module last so persisted selection wins.
    std::vector<std::string> uiModuleIDs(
        state.enabledUIModuleIDs.begin(),
        state.enabledUIModuleIDs.end());

    if (state.selectedUIModuleID.has_value()) {
        const auto& selectedID = state.selectedUIModuleID.value();
        auto selectedIt = std::find(uiModuleIDs.begin(), uiModuleIDs.end(), selectedID);
        if (selectedIt != uiModuleIDs.end()) {
            uiModuleIDs.erase(selectedIt);
            uiModuleIDs.push_back(selectedID);
        }
    }

    for (const auto& moduleID : uiModuleIDs) {
        restoreOne(moduleID);
    }

    if (hadPersistedActivation) {
        persistState();
    }

    lastRestoreResult_ = result;
    return result;
}

// ---------------------------------------------------------------------------
// Getters
// ---------------------------------------------------------------------------

const std::set<std::string>& ModuleManager::enabledServiceModuleIDs() const
{
    return enabledServiceModuleIDs_;
}

const std::set<std::string>& ModuleManager::enabledUIModuleIDs() const
{
    return enabledUIModuleIDs_;
}

const std::optional<std::string>& ModuleManager::activeUIModuleID() const
{
    return activeUIModuleID_;
}

const std::unordered_map<std::string, ModuleManifest>& ModuleManager::manifestsByID() const
{
    return manifestsByID_;
}

bool ModuleManager::isModuleActive(const std::string& moduleID) const
{
    return loadedModules_.find(moduleID) != loadedModules_.end();
}

ActivationRestoreResult ModuleManager::lastRestoreResult() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return lastRestoreResult_;
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void ModuleManager::validateResolvedModule(const IForsettiModule& module, const ModuleManifest& manifest) const
{
    const auto descriptor = module.descriptor();

    if (descriptor.moduleID != manifest.moduleID) {
        throw ModuleManagerException(
            ModuleManagerError::ModuleIdentityMismatch,
            "Module descriptor ID does not match activation manifest: " + manifest.moduleID);
    }

    if (descriptor.type != manifest.moduleType) {
        throw ModuleManagerException(
            ModuleManagerError::ModuleTypeMismatch,
            "Module descriptor type does not match activation manifest: " + manifest.moduleID);
    }

    if (!(descriptor.version == manifest.moduleVersion)) {
        throw ModuleManagerException(
            ModuleManagerError::ModuleVersionMismatch,
            "Module descriptor version does not match activation manifest: " + manifest.moduleID);
    }

    const auto bundledManifest = module.manifest();
    if (!(bundledManifest == manifest)) {
        throw ModuleManagerException(
            ModuleManagerError::ModuleManifestMismatch,
            "Module bundled manifest does not match activation manifest: " + manifest.moduleID);
    }
}

std::shared_ptr<ForsettiContext> ModuleManager::makeModuleContext(const ModuleManifest& manifest) const
{
    return context_->scopedToModule(manifest.moduleID, manifest.capabilitiesRequested);
}

void ModuleManager::persistState()
{
    ActivationState state;
    state.enabledServiceModuleIDs = enabledServiceModuleIDs_;
    state.enabledUIModuleIDs = enabledUIModuleIDs_;
    state.selectedUIModuleID = activeUIModuleID_;

    store_->saveState(state);
}

UIContributions ModuleManager::sanitizedUIContributions(const UIContributions& original) const
{
    UIContributions sanitized = original;
    sanitized.themeMask = std::nullopt;
    return sanitized;
}

} // namespace Forsetti
