// Forsetti Framework for Windows
// Copyright (c) 2026 James Daley. All Rights Reserved.
// Proprietary and Confidential. Patent Pending.

#pragma once

#include "ForsettiCore/ModuleRegistration.h"

#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace Forsetti {

class RegistryModuleRegistrationStore final : public IModuleRegistrationStore {
public:
    RegistryModuleRegistrationStore();
    explicit RegistryModuleRegistrationStore(std::wstring rootSubkey);

    std::optional<ModuleRegistrationRecord> load(const std::string& moduleID) const override;
    std::vector<ModuleRegistrationRecord> loadAll() const override;
    void save(const ModuleRegistrationRecord& record) override;
    void remove(const std::string& moduleID) override;

private:
    std::wstring rootSubkey_;
    mutable std::mutex mutex_;
};

class CngSha256ManifestDigestProvider final : public IManifestDigestProvider {
public:
    std::string canonicalManifestJSON(const ModuleManifest& manifest) const override;
    std::string digestManifest(const ModuleManifest& manifest) const override;
};

} // namespace Forsetti
