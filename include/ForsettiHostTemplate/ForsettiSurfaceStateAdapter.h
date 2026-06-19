// Forsetti Framework for Windows
// Copyright (c) 2026 James Daley. All Rights Reserved.
// Proprietary and Confidential. Patent Pending.

#pragma once

#include "ForsettiCore/UISurfaceManager.h"

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace Forsetti {

struct ForsettiSurfaceStateSnapshot final {
    std::optional<ThemeMask> themeMask;
    std::vector<ToolbarItemDescriptor> toolbarItems;
    std::map<std::string, std::vector<ViewInjectionDescriptor>> viewInjectionsBySlot;
    std::optional<OverlaySchema> overlaySchema;

    bool operator==(const ForsettiSurfaceStateSnapshot&) const = default;
};

class IHostSurfaceStateSource {
public:
    virtual ForsettiSurfaceStateSnapshot snapshot() const = 0;
    virtual ~IHostSurfaceStateSource() = default;
};

class ForsettiSurfaceStateAdapter final : public IHostSurfaceStateSource {
public:
    explicit ForsettiSurfaceStateAdapter(std::shared_ptr<UISurfaceManager> surfaceManager);

    ForsettiSurfaceStateSnapshot snapshot() const override;

private:
    std::shared_ptr<UISurfaceManager> surfaceManager_;
};

} // namespace Forsetti
