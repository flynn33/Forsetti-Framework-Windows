// Forsetti Framework for Windows
// Copyright (c) 2026 James Daley.
// SPDX-License-Identifier: Apache-2.0

#include "ForsettiHostTemplate/ForsettiSurfaceStateAdapter.h"

#include <stdexcept>
#include <utility>

namespace Forsetti {

ForsettiSurfaceStateAdapter::ForsettiSurfaceStateAdapter(
    std::shared_ptr<UISurfaceManager> surfaceManager)
    : surfaceManager_(std::move(surfaceManager))
{
    if (!surfaceManager_) {
        throw std::invalid_argument("ForsettiSurfaceStateAdapter requires a surface manager.");
    }
}

ForsettiSurfaceStateSnapshot ForsettiSurfaceStateAdapter::snapshot() const
{
    return ForsettiSurfaceStateSnapshot{
        .themeMask = surfaceManager_->currentThemeMask(),
        .toolbarItems = surfaceManager_->currentToolbarItems(),
        .viewInjectionsBySlot = surfaceManager_->currentViewInjectionsBySlot(),
        .overlaySchema = surfaceManager_->currentOverlaySchema()
    };
}

} // namespace Forsetti
