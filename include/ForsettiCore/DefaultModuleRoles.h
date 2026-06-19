// Forsetti Framework for Windows
// Copyright (c) 2026 James Daley. All Rights Reserved.
// Proprietary and Confidential. Patent Pending.

#pragma once

#include "ForsettiCore/ModuleRegistration.h"

#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace Forsetti {

enum class DefaultModuleRoleResolutionError {
    MissingProvider,
    AmbiguousProvider,
    SelectedProviderUnavailable
};

class DefaultModuleRoleException final : public std::runtime_error {
public:
    DefaultModuleRoleException(DefaultModuleRoleResolutionError error, std::string message);

    [[nodiscard]] DefaultModuleRoleResolutionError error() const noexcept;

private:
    DefaultModuleRoleResolutionError error_;
};

class IDefaultModuleCatalog {
public:
    virtual std::vector<ModuleRegistrationRecord> providersFor(DefaultModuleRole role) const = 0;
    virtual ModuleRegistrationRecord resolve(
        DefaultModuleRole role,
        const std::optional<std::string>& selectedModuleID = std::nullopt) const = 0;

    virtual ~IDefaultModuleCatalog() = default;
};

class DefaultModuleCatalog final : public IDefaultModuleCatalog {
public:
    explicit DefaultModuleCatalog(std::vector<ModuleRegistrationRecord> records);

    std::vector<ModuleRegistrationRecord> providersFor(DefaultModuleRole role) const override;
    ModuleRegistrationRecord resolve(
        DefaultModuleRole role,
        const std::optional<std::string>& selectedModuleID = std::nullopt) const override;

private:
    std::map<DefaultModuleRole, std::vector<ModuleRegistrationRecord>> providersByRole_;
};

class DefaultModuleOrchestrator final {
public:
    explicit DefaultModuleOrchestrator(std::shared_ptr<IDefaultModuleCatalog> catalog);

    std::vector<ModuleRegistrationRecord> resolveRequiredRoles(
        const ModuleManifest& manifest) const;

private:
    std::shared_ptr<IDefaultModuleCatalog> catalog_;
};

} // namespace Forsetti
