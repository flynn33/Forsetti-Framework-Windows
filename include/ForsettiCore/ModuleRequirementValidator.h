// Forsetti Framework for Windows
// Copyright (c) 2026 James Daley. All Rights Reserved.
// Proprietary and Confidential. Patent Pending.

#pragma once

#include "ForsettiCore/ForsettiServiceContainer.h"
#include "ForsettiCore/ModuleModels.h"

#include <string>
#include <vector>

namespace Forsetti {

enum class ModuleRequirementSeverity {
    Warning,
    Error
};

enum class ModuleRequirementIssueCode {
    MissingCapability,
    MissingProvider
};

struct ModuleRequirementIssue final {
    std::string moduleID;
    std::string requirementID;
    ModuleIOKind kind{ModuleIOKind::Storage};
    ModuleRequirementSeverity severity{ModuleRequirementSeverity::Error};
    ModuleRequirementIssueCode issueCode{ModuleRequirementIssueCode::MissingProvider};
    std::string message;

    bool operator==(const ModuleRequirementIssue&) const = default;
};

struct ModuleRequirementValidationResult final {
    std::vector<ModuleRequirementIssue> issues;

    [[nodiscard]] bool hasErrors() const noexcept;
};

class ModuleRequirementValidator final {
public:
    ModuleRequirementValidationResult validate(
        const ModuleManifest& manifest,
        const IServiceProvider& scopedServices) const;

private:
    [[nodiscard]] bool canResolveService(
        ModuleIOKind kind,
        const IServiceProvider& scopedServices) const;
};

} // namespace Forsetti
