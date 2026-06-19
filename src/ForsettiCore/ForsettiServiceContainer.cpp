// Forsetti Framework for Windows
// Copyright (c) 2026 James Daley. All Rights Reserved.
// Proprietary and Confidential. Patent Pending.

#include "ForsettiCore/ForsettiServiceContainer.h"
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace Forsetti {

namespace {

std::string encodeNamespaceComponent(const std::string& value)
{
    std::ostringstream stream;
    stream << std::hex << std::setfill('0') << std::nouppercase;
    for (const unsigned char ch : value) {
        if (std::isalnum(ch) != 0 || ch == '-' || ch == '_') {
            stream << static_cast<char>(ch);
        } else {
            stream << "_" << std::setw(2) << static_cast<int>(ch);
        }
    }
    return stream.str();
}

bool isReservedDeviceName(const std::string& value)
{
    auto base = value;
    const auto dot = base.find('.');
    if (dot != std::string::npos) {
        base = base.substr(0, dot);
    }
    std::transform(base.begin(), base.end(), base.begin(), [](unsigned char ch) {
        return static_cast<char>(std::toupper(ch));
    });

    if (base == "CON" || base == "PRN" || base == "AUX" || base == "NUL") {
        return true;
    }
    if (base.size() == 4 && (base.starts_with("COM") || base.starts_with("LPT")) &&
        base[3] >= '1' && base[3] <= '9') {
        return true;
    }
    return false;
}

void validateCallerKey(const std::string& key, const std::string& field)
{
    if (key.empty()) {
        throw std::invalid_argument(field + " must not be empty.");
    }
    if (key == "." || key == ".." || key.find("..") != std::string::npos) {
        throw std::invalid_argument(field + " must not contain dot segments.");
    }
    if (key.find('/') != std::string::npos || key.find('\\') != std::string::npos ||
        key.find(':') != std::string::npos) {
        throw std::invalid_argument(field + " must not contain path separators, drive prefixes, or device syntax.");
    }
    if (key.back() == ' ' || key.back() == '.') {
        throw std::invalid_argument(field + " must not end with a space or dot.");
    }
    if (std::any_of(key.begin(), key.end(), [](unsigned char ch) {
        return std::iscntrl(ch) != 0;
    })) {
        throw std::invalid_argument(field + " must not contain control characters.");
    }
    if (isReservedDeviceName(key)) {
        throw std::invalid_argument(field + " must not use a reserved Windows device name.");
    }
}

} // namespace

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
    if (!requiredCapability.has_value()) {
        if (logger_) {
            logger_->log(
                LogLevel::Warning,
                "Service type is not exposed to module scope.",
                moduleID_,
                {{"serviceType", type.name()}});
        }
        return std::any{};
    }

    if (!grantedCapabilities_.contains(requiredCapability.value())) {
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

    if (!inner_) {
        return std::any{};
    }

    auto resolved = inner_->resolveAny(type);
    if (!resolved.has_value()) {
        return std::any{};
    }

    if (type == std::type_index(typeid(IStorageService))) {
        return std::make_shared<ScopedStorageService>(
            moduleID_,
            std::any_cast<std::shared_ptr<IStorageService>>(resolved));
    }
    if (type == std::type_index(typeid(ISecureStorageService))) {
        return std::make_shared<ScopedSecureStorageService>(
            moduleID_,
            std::any_cast<std::shared_ptr<ISecureStorageService>>(resolved));
    }
    if (type == std::type_index(typeid(IFileExportService))) {
        return std::make_shared<ScopedFileExportService>(
            moduleID_,
            std::any_cast<std::shared_ptr<IFileExportService>>(resolved));
    }

    return resolved;
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
    if (type == std::type_index(typeid(ISharedDatabaseService))) {
        return Capability::SharedDatabase;
    }
    if (type == std::type_index(typeid(IAuthenticationService))) {
        return Capability::Authentication;
    }
    if (type == std::type_index(typeid(IDiagnosticsService))) {
        return Capability::Diagnostics;
    }
    if (type == std::type_index(typeid(IApiService))) {
        return Capability::API;
    }
    if (type == std::type_index(typeid(ISecurityService))) {
        return Capability::Security;
    }
    if (type == std::type_index(typeid(ICryptoUtilitiesService))) {
        return Capability::CryptoUtilities;
    }
    return std::nullopt;
}

ScopedStorageService::ScopedStorageService(
    std::string moduleID,
    std::shared_ptr<IStorageService> inner)
    : moduleID_(std::move(moduleID))
    , inner_(std::move(inner))
{
}

void ScopedStorageService::set(const std::string& key, const std::string& value)
{
    inner_->set(scopedKey(key), value);
}

std::optional<std::string> ScopedStorageService::get(const std::string& key)
{
    return inner_->get(scopedKey(key));
}

void ScopedStorageService::remove(const std::string& key)
{
    inner_->remove(scopedKey(key));
}

std::string ScopedStorageService::scopedKey(const std::string& key) const
{
    validateCallerKey(key, "storage key");
    return "module." + encodeNamespaceComponent(moduleID_) + "." + key;
}

ScopedSecureStorageService::ScopedSecureStorageService(
    std::string moduleID,
    std::shared_ptr<ISecureStorageService> inner)
    : moduleID_(std::move(moduleID))
    , inner_(std::move(inner))
{
}

void ScopedSecureStorageService::set(const std::string& key, const std::vector<uint8_t>& data)
{
    inner_->set(scopedKey(key), data);
}

std::optional<std::vector<uint8_t>> ScopedSecureStorageService::get(const std::string& key)
{
    return inner_->get(scopedKey(key));
}

void ScopedSecureStorageService::remove(const std::string& key)
{
    inner_->remove(scopedKey(key));
}

std::string ScopedSecureStorageService::scopedKey(const std::string& key) const
{
    validateCallerKey(key, "secure storage key");
    return "module." + encodeNamespaceComponent(moduleID_) + "." + key;
}

ScopedFileExportService::ScopedFileExportService(
    std::string moduleID,
    std::shared_ptr<IFileExportService> inner)
    : moduleID_(std::move(moduleID))
    , inner_(std::move(inner))
{
}

bool ScopedFileExportService::exportData(
    const std::vector<uint8_t>& data,
    const std::string& filename)
{
    return inner_->exportData(data, scopedFilename(filename));
}

std::string ScopedFileExportService::scopedFilename(const std::string& filename) const
{
    validateCallerKey(filename, "export filename");
    return encodeNamespaceComponent(moduleID_) + "-" + filename;
}

} // namespace Forsetti
