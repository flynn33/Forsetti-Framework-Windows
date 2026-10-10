// Forsetti Framework for Windows
// Copyright (c) 2026 James Daley.
// SPDX-License-Identifier: Apache-2.0

#include "ForsettiHostTemplate/ForsettiHostOverlayRouter.h"

namespace Forsetti {

void ForsettiHostOverlayRouter::openPointer(const std::string& pointerID)
{
    std::lock_guard lock(mutex_);
    pointerID_ = pointerID;
    routeID_.reset();
}

void ForsettiHostOverlayRouter::openRoute(const std::string& routeID)
{
    std::lock_guard lock(mutex_);
    routeID_ = routeID;
    pointerID_.reset();
}

std::optional<std::string> ForsettiHostOverlayRouter::currentPointerID() const
{
    std::lock_guard lock(mutex_);
    return pointerID_;
}

std::optional<std::string> ForsettiHostOverlayRouter::currentRouteID() const
{
    std::lock_guard lock(mutex_);
    return routeID_;
}

} // namespace Forsetti
