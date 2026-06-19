// Forsetti Framework for Windows
// Copyright (c) 2026 James Daley. All Rights Reserved.
// Proprietary and Confidential. Patent Pending.

#include "ForsettiHostTemplate/ForsettiHostController.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace Forsetti {

ForsettiHostController::ForsettiHostController(std::shared_ptr<ForsettiRuntime> runtime)
    : runtime_(std::move(runtime))
{
    if (!runtime_) {
        throw std::invalid_argument("ForsettiHostController requires a runtime.");
    }
}

void ForsettiHostController::boot(ForsettiHostLaunchStrategy launchStrategy)
{
    {
        std::lock_guard lock(mutex_);
        refreshSnapshotLocked(ForsettiHostStateKind::Booting);
    }

    try {
        runtime_->boot();
        applyLaunchStrategy(launchStrategy);

        std::lock_guard lock(mutex_);
        refreshSnapshotLocked(runtime_->moduleManager().activeUIModuleID().has_value()
            ? ForsettiHostStateKind::Active
            : ForsettiHostStateKind::ReadyWithoutActiveUI);
    } catch (const std::exception& ex) {
        std::lock_guard lock(mutex_);
        recordFailureLocked(ForsettiHostStateKind::FatalError, ex.what());
        throw;
    }
}

void ForsettiHostController::shutdown()
{
    {
        std::lock_guard lock(mutex_);
        refreshSnapshotLocked(ForsettiHostStateKind::ShuttingDown);
    }

    runtime_->shutdown();

    std::lock_guard lock(mutex_);
    refreshSnapshotLocked(ForsettiHostStateKind::NotStarted);
}

void ForsettiHostController::activateModule(const std::string& moduleID)
{
    {
        std::lock_guard lock(mutex_);
        refreshSnapshotLocked(ForsettiHostStateKind::Activating);
    }

    try {
        runtime_->activateModule(moduleID);
        std::lock_guard lock(mutex_);
        refreshSnapshotLocked(runtime_->moduleManager().activeUIModuleID().has_value()
            ? ForsettiHostStateKind::Active
            : ForsettiHostStateKind::ReadyWithoutActiveUI);
    } catch (const std::exception& ex) {
        std::lock_guard lock(mutex_);
        recordFailureLocked(ForsettiHostStateKind::RecoverableError, ex.what());
        throw;
    }
}

void ForsettiHostController::deactivateModule(const std::string& moduleID)
{
    runtime_->deactivateModule(moduleID);

    std::lock_guard lock(mutex_);
    refreshSnapshotLocked(runtime_->moduleManager().activeUIModuleID().has_value()
        ? ForsettiHostStateKind::Active
        : ForsettiHostStateKind::ReadyWithoutActiveUI);
}

ForsettiHostStateSnapshot ForsettiHostController::snapshot() const
{
    std::lock_guard lock(mutex_);
    return snapshot_;
}

void ForsettiHostController::applyLaunchStrategy(const ForsettiHostLaunchStrategy& launchStrategy)
{
    if (launchStrategy.kind == ForsettiHostLaunchStrategyKind::RestoreOnly) {
        return;
    }

    if (launchStrategy.kind == ForsettiHostLaunchStrategyKind::ExplicitModuleIDs) {
        for (const auto& moduleID : launchStrategy.moduleIDs) {
            runtime_->activateModule(moduleID);
        }
        return;
    }

    std::vector<std::string> moduleIDs;
    for (const auto& [moduleID, manifest] : runtime_->moduleManager().manifestsByID()) {
        if (manifest.moduleType == ModuleType::Service) {
            moduleIDs.push_back(moduleID);
        }
    }
    std::sort(moduleIDs.begin(), moduleIDs.end());
    for (const auto& moduleID : moduleIDs) {
        runtime_->activateModule(moduleID);
    }
}

void ForsettiHostController::refreshSnapshotLocked(ForsettiHostStateKind state)
{
    snapshot_.state = state;
    snapshot_.availableModuleIDs.clear();
    snapshot_.activeModuleIDs.clear();
    snapshot_.activeUIModuleID = runtime_->moduleManager().activeUIModuleID();
    snapshot_.lastError = std::nullopt;

    for (const auto& [moduleID, manifest] : runtime_->moduleManager().manifestsByID()) {
        (void)manifest;
        snapshot_.availableModuleIDs.push_back(moduleID);
    }
    std::sort(snapshot_.availableModuleIDs.begin(), snapshot_.availableModuleIDs.end());

    snapshot_.activeModuleIDs.insert(
        snapshot_.activeModuleIDs.end(),
        runtime_->moduleManager().enabledServiceModuleIDs().begin(),
        runtime_->moduleManager().enabledServiceModuleIDs().end());
    snapshot_.activeModuleIDs.insert(
        snapshot_.activeModuleIDs.end(),
        runtime_->moduleManager().enabledUIModuleIDs().begin(),
        runtime_->moduleManager().enabledUIModuleIDs().end());
    std::sort(snapshot_.activeModuleIDs.begin(), snapshot_.activeModuleIDs.end());
}

void ForsettiHostController::recordFailureLocked(
    ForsettiHostStateKind state,
    const std::string& message)
{
    refreshSnapshotLocked(state);
    snapshot_.lastError = message;
}

} // namespace Forsetti
