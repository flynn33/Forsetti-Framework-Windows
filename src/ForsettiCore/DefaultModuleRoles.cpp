// Forsetti Framework for Windows
// Copyright (c) 2026 James Daley. All Rights Reserved.
// Proprietary and Confidential. Patent Pending.

#include "ForsettiCore/DefaultModuleRoles.h"

#include <algorithm>
#include <utility>

namespace Forsetti {

DefaultModuleRoleException::DefaultModuleRoleException(
    DefaultModuleRoleResolutionError error,
    std::string message)
    : std::runtime_error(std::move(message))
    , error_(error)
{
}

DefaultModuleRoleResolutionError DefaultModuleRoleException::error() const noexcept
{
    return error_;
}

DefaultModuleCatalog::DefaultModuleCatalog(std::vector<ModuleRegistrationRecord> records)
{
    for (auto& record : records) {
        if (record.confirmed && record.defaultModuleRole.has_value()) {
            providersByRole_[record.defaultModuleRole.value()].push_back(std::move(record));
        }
    }

    for (auto& [role, providers] : providersByRole_) {
        (void)role;
        std::sort(providers.begin(), providers.end(), [](const auto& lhs, const auto& rhs) {
            return lhs.moduleID < rhs.moduleID;
        });
    }
}

std::vector<ModuleRegistrationRecord> DefaultModuleCatalog::providersFor(
    DefaultModuleRole role) const
{
    const auto it = providersByRole_.find(role);
    if (it == providersByRole_.end()) {
        return {};
    }
    return it->second;
}

ModuleRegistrationRecord DefaultModuleCatalog::resolve(
    DefaultModuleRole role,
    const std::optional<std::string>& selectedModuleID) const
{
    const auto providers = providersFor(role);
    if (selectedModuleID.has_value()) {
        const auto selected = std::find_if(
            providers.begin(),
            providers.end(),
            [&selectedModuleID](const ModuleRegistrationRecord& record) {
                return record.moduleID == selectedModuleID.value();
            });
        if (selected == providers.end()) {
            throw DefaultModuleRoleException(
                DefaultModuleRoleResolutionError::SelectedProviderUnavailable,
                "Selected provider is unavailable for role " + to_string(role) + ": " + selectedModuleID.value());
        }
        return *selected;
    }

    if (providers.empty()) {
        throw DefaultModuleRoleException(
            DefaultModuleRoleResolutionError::MissingProvider,
            "No confirmed provider is available for role " + to_string(role));
    }

    if (providers.size() > 1) {
        throw DefaultModuleRoleException(
            DefaultModuleRoleResolutionError::AmbiguousProvider,
            "Multiple confirmed providers are available for role " + to_string(role));
    }

    return providers[0];
}

DefaultModuleOrchestrator::DefaultModuleOrchestrator(
    std::shared_ptr<IDefaultModuleCatalog> catalog)
    : catalog_(std::move(catalog))
{
    if (!catalog_) {
        throw std::invalid_argument("DefaultModuleOrchestrator requires a catalog.");
    }
}

std::vector<ModuleRegistrationRecord> DefaultModuleOrchestrator::resolveRequiredRoles(
    const ModuleManifest& manifest) const
{
    std::vector<ModuleRegistrationRecord> providers;
    for (const auto role : manifest.runtimeRequirements.dataIsolation.requiredDefaultRoles) {
        providers.push_back(catalog_->resolve(role));
    }
    return providers;
}

} // namespace Forsetti
