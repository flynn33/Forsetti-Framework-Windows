// Forsetti Framework for Windows
// Copyright (c) 2026 James Daley. All Rights Reserved.
// Proprietary and Confidential. Patent Pending.

#pragma once

#include "ForsettiCore/ForsettiContext.h"

#include <mutex>
#include <optional>
#include <string>

namespace Forsetti {

class IForsettiHostOverlayRouter : public IOverlayRouter {
public:
    virtual std::optional<std::string> currentPointerID() const = 0;
    virtual std::optional<std::string> currentRouteID() const = 0;
};

class ForsettiHostOverlayRouter final : public IForsettiHostOverlayRouter {
public:
    void openPointer(const std::string& pointerID) override;
    void openRoute(const std::string& routeID) override;

    std::optional<std::string> currentPointerID() const override;
    std::optional<std::string> currentRouteID() const override;

private:
    mutable std::mutex mutex_;
    std::optional<std::string> pointerID_;
    std::optional<std::string> routeID_;
};

} // namespace Forsetti
