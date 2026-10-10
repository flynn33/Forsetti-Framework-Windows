// Forsetti Framework for Windows
// Copyright (c) 2026 James Daley.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include "ForsettiHostTemplate/ForsettiHostController.h"
#include "ForsettiHostTemplate/ForsettiSurfaceStateAdapter.h"
#include "ForsettiPlatform/WindowsViewFactoryRegistry.h"
#include <Windows.h>
#include <map>
#include <memory>
#include <string>
#include <vector>

class NativePresentation final {
public:
    NativePresentation(std::shared_ptr<Forsetti::IForsettiHostController> controller,
        std::shared_ptr<Forsetti::IHostSurfaceStateSource> surface,
        const std::vector<Forsetti::ModuleManifest>& manifests);
    ~NativePresentation();
    void render();
    bool hasPresentedView(const std::string& expectedViewID) const;
    int run();
private:
    static LRESULT CALLBACK windowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
    void command(unsigned int id);
    HWND window_ = nullptr;
    HWND status_ = nullptr;
    std::vector<std::string> moduleIDs_;
    std::shared_ptr<Forsetti::IForsettiHostController> controller_;
    std::shared_ptr<Forsetti::IHostSurfaceStateSource> surface_;
    Forsetti::ForsettiViewFactoryRegistry factories_;
    std::shared_ptr<void> presentedView_;
    std::string presentedViewID_;
};
