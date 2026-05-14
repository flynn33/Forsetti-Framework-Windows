// Forsetti Framework for Windows
// Copyright (c) 2026 James Daley. All Rights Reserved.
// Proprietary and Confidential. Patent Pending.

#pragma once

#include "ForsettiCore/ForsettiLogger.h"
#include "ForsettiCore/ForsettiServices.h"
#include "ForsettiCore/ModuleModels.h"

#include <any>
#include <typeindex>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <optional>
#include <set>
#include <string>

namespace Forsetti {

class IServiceProvider {
public:
    virtual std::any resolveAny(const std::type_index& type) const = 0;

    template<typename T>
    std::shared_ptr<T> resolve() const {
        auto result = resolveAny(std::type_index(typeid(T)));
        if (!result.has_value()) {
            return nullptr;
        }
        return std::any_cast<std::shared_ptr<T>>(result);
    }

    virtual ~IServiceProvider() = default;
};

class ServiceContainer final : public IServiceProvider {
public:
    template<typename T>
    void registerService(std::shared_ptr<T> service) {
        std::lock_guard<std::mutex> lock(mutex_);
        services_[std::type_index(typeid(T))] = std::move(service);
    }

    std::any resolveAny(const std::type_index& type) const override;

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::type_index, std::any> services_;
};

class CapabilityScopedServiceProvider final : public IServiceProvider {
public:
    CapabilityScopedServiceProvider(
        std::shared_ptr<IServiceProvider> inner,
        std::string moduleID,
        std::set<Capability> grantedCapabilities,
        std::shared_ptr<IForsettiLogger> logger);

    std::any resolveAny(const std::type_index& type) const override;

    [[nodiscard]] const std::string& moduleID() const noexcept;
    [[nodiscard]] const std::set<Capability>& grantedCapabilities() const noexcept;

private:
    [[nodiscard]] std::optional<Capability> requiredCapabilityForService(
        const std::type_index& type) const;

    std::shared_ptr<IServiceProvider> inner_;
    std::string moduleID_;
    std::set<Capability> grantedCapabilities_;
    std::shared_ptr<IForsettiLogger> logger_;
};

} // namespace Forsetti
