// Forsetti Framework for Windows
// Copyright (c) 2026 James Daley. All Rights Reserved.
// Proprietary and Confidential. Patent Pending.

#pragma once
#include "ForsettiCore/ForsettiServices.h"

#include <filesystem>
#include <mutex>

namespace Forsetti {

// ---------------------------------------------------------------------------
// WinHttpNetworkingService — Windows HTTP networking via WinHTTP.
// ---------------------------------------------------------------------------
class WinHttpNetworkingService final : public INetworkingService {
public:
    std::future<std::vector<uint8_t>> data(
        const std::string& url,
        const std::map<std::string, std::string>& headers = {}) override;
};

// ---------------------------------------------------------------------------
// RegistryStorageService — Persistent storage backed by Windows Registry.
// ---------------------------------------------------------------------------
class RegistryStorageService final : public IStorageService {
public:
    RegistryStorageService();
    explicit RegistryStorageService(std::wstring rootSubkey);

    void set(const std::string& key, const std::string& value) override;
    std::optional<std::string> get(const std::string& key) override;
    void remove(const std::string& key) override;

private:
    std::wstring rootSubkey_;
    mutable std::mutex mutex_;
};

// ---------------------------------------------------------------------------
// DpapiSecureStorageService — Secure storage backed by Windows DPAPI.
// ---------------------------------------------------------------------------
class DpapiSecureStorageService final : public ISecureStorageService {
public:
    DpapiSecureStorageService();
    explicit DpapiSecureStorageService(std::wstring rootSubkey);

    void set(const std::string& key, const std::vector<uint8_t>& data) override;
    std::optional<std::vector<uint8_t>> get(const std::string& key) override;
    void remove(const std::string& key) override;

private:
    std::wstring rootSubkey_;
    mutable std::mutex mutex_;
};

// ---------------------------------------------------------------------------
// LocalFileExportService — File export to the local filesystem.
// ---------------------------------------------------------------------------
class LocalFileExportService final : public IFileExportService {
public:
    LocalFileExportService();
    explicit LocalFileExportService(std::filesystem::path exportDirectory);

    bool exportData(const std::vector<uint8_t>& data,
                    const std::string& filename) override;

    [[nodiscard]] const std::filesystem::path& exportDirectory() const noexcept;

    [[nodiscard]] static std::filesystem::path defaultExportDirectory();
    [[nodiscard]] static std::string sanitizedFilename(const std::string& filename);

private:
    std::filesystem::path exportDirectory_;
};

// ---------------------------------------------------------------------------
// NoopTelemetryService — No-op telemetry (same as Swift).
// All calls are silently ignored.
// ---------------------------------------------------------------------------
class NoopTelemetryService final : public ITelemetryService {
public:
    void trackEvent(const std::string& name,
                    const std::map<std::string, std::string>& properties = {}) override;
};

} // namespace Forsetti
