// Forsetti Framework for Windows
// Copyright (c) 2026 James Daley. All Rights Reserved.
// Proprietary and Confidential. Patent Pending.

#include "ForsettiHostTemplate/ForsettiHostState.h"

#include <utility>

namespace Forsetti {

ForsettiHostLaunchStrategy ForsettiHostLaunchStrategy::restoreOnly()
{
    return ForsettiHostLaunchStrategy{};
}

ForsettiHostLaunchStrategy ForsettiHostLaunchStrategy::activateAllEligibleForDevelopment()
{
    return ForsettiHostLaunchStrategy{
        .kind = ForsettiHostLaunchStrategyKind::ActivateAllEligibleForDevelopment
    };
}

ForsettiHostLaunchStrategy ForsettiHostLaunchStrategy::explicitModuleIDs(std::vector<std::string> ids)
{
    return ForsettiHostLaunchStrategy{
        .kind = ForsettiHostLaunchStrategyKind::ExplicitModuleIDs,
        .moduleIDs = std::move(ids)
    };
}

std::string to_string(ForsettiHostStateKind state)
{
    switch (state) {
        case ForsettiHostStateKind::NotStarted: return "not_started";
        case ForsettiHostStateKind::Booting: return "booting";
        case ForsettiHostStateKind::ReadyWithoutActiveUI: return "ready_without_active_ui";
        case ForsettiHostStateKind::Activating: return "activating";
        case ForsettiHostStateKind::Active: return "active";
        case ForsettiHostStateKind::RecoverableError: return "recoverable_error";
        case ForsettiHostStateKind::FatalError: return "fatal_error";
        case ForsettiHostStateKind::ShuttingDown: return "shutting_down";
    }
    return "unknown";
}

std::string to_string(ForsettiHostLaunchStrategyKind strategy)
{
    switch (strategy) {
        case ForsettiHostLaunchStrategyKind::RestoreOnly: return "restore_only";
        case ForsettiHostLaunchStrategyKind::ActivateAllEligibleForDevelopment:
            return "activate_all_eligible_for_development";
        case ForsettiHostLaunchStrategyKind::ExplicitModuleIDs: return "explicit_module_ids";
    }
    return "unknown";
}

} // namespace Forsetti
