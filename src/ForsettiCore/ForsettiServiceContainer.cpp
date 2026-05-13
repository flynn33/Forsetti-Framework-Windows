// Forsetti Framework for Windows
// Copyright (c) 2026 James Daley. All Rights Reserved.
// Proprietary and Confidential. Patent Pending.

#include "ForsettiCore/ForsettiServiceContainer.h"
#include <utility>

namespace Forsetti {

std::any ServiceContainer::resolveAny(const std::type_index& type) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = services_.find(type);
    if (it != services_.end()) {
        return it->second;
    }
    return std::any{};
}

CapabilityScopedServiceProvider::CapabilityScopedServiceProvider(
    std::shared_ptr<IServiceProvider> inner,
    std::string moduleID,
    std::set<Capability> grantedCapabilities,
    std::shared_ptr<IForsettiLogger> logger)
    : inner_(std::move(inner))
    , moduleID_(std::move(moduleID))
    , grantedCapabilities_(std::move(grantedCapabilities))
    , logger_(std::move(logger))
{
}

std::any CapabilityScopedServiceProvider::resolveAny(const std::type_index& type) const {
    const auto requiredCapability = requiredCapabilityForService(type);
    if (requiredCapability.has_value() &&
        !grantedCapabilities_.contains(requiredCapability.value())) {
        if (logger_) {
            logger_->log(
                LogLevel::Warning,
                "Capability denied: " + to_string(requiredCapability.value()),
                moduleID_,
                {
                    {"capability", to_string(requiredCapability.value())},
                    {"serviceType", type.name()}
                });
        }
        return std::any{};
    }

    return inner_ ? inner_->resolveAny(type) : std::any{};
}

const std::string& CapabilityScopedServiceProvider::moduleID() const noexcept {
    return moduleID_;
}

const std::set<Capability>& CapabilityScopedServiceProvider::grantedCapabilities() const noexcept {
    return grantedCapabilities_;
}

std::optional<Capability> CapabilityScopedServiceProvider::requiredCapabilityForService(
    const std::type_index& type) const {
    if (type == std::type_index(typeid(INetworkingService))) {
        return Capability::Networking;
    }
    if (type == std::type_index(typeid(IStorageService))) {
        return Capability::Storage;
    }
    if (type == std::type_index(typeid(ISecureStorageService))) {
        return Capability::SecureStorage;
    }
    if (type == std::type_index(typeid(IFileExportService))) {
        return Capability::FileExport;
    }
    if (type == std::type_index(typeid(ITelemetryService))) {
        return Capability::Telemetry;
    }
    return std::nullopt;
}

} // namespace Forsetti
