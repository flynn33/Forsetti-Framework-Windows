// Forsetti Framework for Windows
// Copyright (c) 2026 James Daley. All Rights Reserved.
// Proprietary and Confidential. Patent Pending.

#pragma once

#include <optional>
#include <string>
#include <vector>

namespace Forsetti {

enum class ForsettiHostStateKind {
    NotStarted,
    Booting,
    ReadyWithoutActiveUI,
    Activating,
    Active,
    RecoverableError,
    FatalError,
    ShuttingDown
};

enum class ForsettiHostLaunchStrategyKind {
    RestoreOnly,
    ActivateAllEligibleForDevelopment,
    ExplicitModuleIDs
};

struct ForsettiHostLaunchStrategy final {
    ForsettiHostLaunchStrategyKind kind{ForsettiHostLaunchStrategyKind::RestoreOnly};
    std::vector<std::string> moduleIDs;

    [[nodiscard]] static ForsettiHostLaunchStrategy restoreOnly();
    [[nodiscard]] static ForsettiHostLaunchStrategy activateAllEligibleForDevelopment();
    [[nodiscard]] static ForsettiHostLaunchStrategy explicitModuleIDs(std::vector<std::string> ids);
};

struct ForsettiHostStateSnapshot final {
    ForsettiHostStateKind state{ForsettiHostStateKind::NotStarted};
    std::vector<std::string> availableModuleIDs;
    std::vector<std::string> activeModuleIDs;
    std::optional<std::string> activeUIModuleID;
    std::optional<std::string> lastError;

    bool operator==(const ForsettiHostStateSnapshot&) const = default;
};

std::string to_string(ForsettiHostStateKind state);
std::string to_string(ForsettiHostLaunchStrategyKind strategy);

} // namespace Forsetti
