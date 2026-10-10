// Forsetti Framework for Windows
// Copyright (c) 2026 James Daley.
// SPDX-License-Identifier: Apache-2.0

#include "ActivationStore.h"
#include <Windows.h>
#include <fstream>
#include <stdexcept>
#include <utility>

FileActivationStore::FileActivationStore(std::filesystem::path path) : path_(std::move(path)) {}
Forsetti::ActivationState FileActivationStore::loadState() const
{
    if (!std::filesystem::exists(path_)) { return {}; }
    std::ifstream stream(path_);
    if (!stream) { throw std::runtime_error("Cannot open application activation state."); }
    nlohmann::json value;
    stream >> value;
    return value.get<Forsetti::ActivationState>();
}
void FileActivationStore::saveState(const Forsetti::ActivationState& state)
{
    std::filesystem::create_directories(path_.parent_path());
    auto temporary = path_;
    temporary += L".tmp";
    {
        std::ofstream stream(temporary, std::ios::trunc);
        if (!stream) { throw std::runtime_error("Cannot write application activation state."); }
        stream << nlohmann::json(state).dump(2);
        stream.flush();
        if (!stream) { throw std::runtime_error("Activation state write failed."); }
    }
    if (!MoveFileExW(temporary.c_str(), path_.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        throw std::runtime_error("Cannot replace application activation state.");
    }
}
