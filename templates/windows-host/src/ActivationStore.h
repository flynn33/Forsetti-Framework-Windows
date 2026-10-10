// Forsetti Framework for Windows
// Copyright (c) 2026 James Daley.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include "ForsettiCore/ActivationStore.h"
#include <filesystem>

class FileActivationStore final : public Forsetti::IActivationStore {
public:
    explicit FileActivationStore(std::filesystem::path path);
    Forsetti::ActivationState loadState() const override;
    void saveState(const Forsetti::ActivationState& state) override;
private:
    std::filesystem::path path_;
};
