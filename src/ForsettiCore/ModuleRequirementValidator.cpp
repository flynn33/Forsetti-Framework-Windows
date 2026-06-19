// Forsetti Framework for Windows
// Copyright (c) 2026 James Daley. All Rights Reserved.
// Proprietary and Confidential. Patent Pending.

#include "ForsettiCore/ModuleRequirementValidator.h"

#include <algorithm>

namespace Forsetti {

bool ModuleRequirementValidationResult::hasErrors() const noexcept
{
    return std::any_of(issues.begin(), issues.end(), [](const ModuleRequirementIssue& issue) {
        return issue.severity == ModuleRequirementSeverity::Error;
    });
}

ModuleRequirementValidationResult ModuleRequirementValidator::validate(
    const ModuleManifest& manifest,
    const IServiceProvider& scopedServices) const
{
    ModuleRequirementValidationResult result;

    for (const auto& requirement : manifest.runtimeRequirements.io) {
        const auto requiredCapability = capabilityForIOKind(requirement.kind);
        const auto hasCapability = std::find(
            manifest.capabilitiesRequested.begin(),
            manifest.capabilitiesRequested.end(),
            requiredCapability) != manifest.capabilitiesRequested.end();

        if (!hasCapability) {
            result.issues.push_back({
                .moduleID = manifest.moduleID,
                .requirementID = requirement.requirementID,
                .kind = requirement.kind,
                .severity = ModuleRequirementSeverity::Error,
                .issueCode = ModuleRequirementIssueCode::MissingCapability,
                .message = "Requirement lacks matching capability: " + to_string(requiredCapability)
            });
            continue;
        }

        if (!canResolveService(requirement.kind, scopedServices)) {
            result.issues.push_back({
                .moduleID = manifest.moduleID,
                .requirementID = requirement.requirementID,
                .kind = requirement.kind,
                .severity = requirement.required
                    ? ModuleRequirementSeverity::Error
                    : ModuleRequirementSeverity::Warning,
                .issueCode = ModuleRequirementIssueCode::MissingProvider,
                .message = "Requirement provider is not available: " + to_string(requirement.kind)
            });
        }
    }

    return result;
}

bool ModuleRequirementValidator::canResolveService(
    ModuleIOKind kind,
    const IServiceProvider& scopedServices) const
{
    switch (kind) {
        case ModuleIOKind::Networking:
            return scopedServices.resolve<INetworkingService>() != nullptr;
        case ModuleIOKind::Storage:
            return scopedServices.resolve<IStorageService>() != nullptr;
        case ModuleIOKind::SecureStorage:
            return scopedServices.resolve<ISecureStorageService>() != nullptr;
        case ModuleIOKind::FileExport:
            return scopedServices.resolve<IFileExportService>() != nullptr;
        case ModuleIOKind::CryptoUtilities:
            return scopedServices.resolve<ICryptoUtilitiesService>() != nullptr;
        case ModuleIOKind::Telemetry:
            return scopedServices.resolve<ITelemetryService>() != nullptr;
        case ModuleIOKind::SharedDatabase:
            return scopedServices.resolve<ISharedDatabaseService>() != nullptr;
        case ModuleIOKind::Authentication:
            return scopedServices.resolve<IAuthenticationService>() != nullptr;
        case ModuleIOKind::Diagnostics:
            return scopedServices.resolve<IDiagnosticsService>() != nullptr;
        case ModuleIOKind::API:
            return scopedServices.resolve<IApiService>() != nullptr;
        case ModuleIOKind::Security:
            return scopedServices.resolve<ISecurityService>() != nullptr;
    }

    return false;
}

} // namespace Forsetti
