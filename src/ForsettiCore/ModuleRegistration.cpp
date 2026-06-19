// Forsetti Framework for Windows
// Copyright (c) 2026 James Daley. All Rights Reserved.
// Proprietary and Confidential. Patent Pending.

#include "ForsettiCore/ModuleRegistration.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <set>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace Forsetti {

namespace {

std::string canonicalJSON(const nlohmann::json& value)
{
    if (value.is_object()) {
        std::vector<std::string> keys;
        keys.reserve(value.size());
        for (const auto& item : value.items()) {
            keys.push_back(item.key());
        }
        std::sort(keys.begin(), keys.end());

        std::string result = "{";
        bool first = true;
        for (const auto& key : keys) {
            if (!first) {
                result += ",";
            }
            first = false;
            result += nlohmann::json(key).dump();
            result += ":";
            result += canonicalJSON(value.at(key));
        }
        result += "}";
        return result;
    }

    if (value.is_array()) {
        std::string result = "[";
        for (std::size_t index = 0; index < value.size(); ++index) {
            if (index > 0) {
                result += ",";
            }
            result += canonicalJSON(value.at(index));
        }
        result += "]";
        return result;
    }

    return value.dump();
}

constexpr std::array<std::uint32_t, 64> sha256RoundConstants = {
    0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u,
    0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
    0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u,
    0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
    0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu,
    0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
    0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u,
    0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
    0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u,
    0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
    0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u,
    0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
    0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u,
    0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
    0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u,
    0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u
};

std::uint32_t rotateRight(std::uint32_t value, std::uint32_t count)
{
    return (value >> count) | (value << (32u - count));
}

std::string sha256Hex(const std::string& input)
{
    std::vector<std::uint8_t> bytes(input.begin(), input.end());
    const auto bitLength = static_cast<std::uint64_t>(bytes.size()) * 8u;

    bytes.push_back(0x80u);
    while ((bytes.size() % 64u) != 56u) {
        bytes.push_back(0u);
    }
    for (int shift = 56; shift >= 0; shift -= 8) {
        bytes.push_back(static_cast<std::uint8_t>((bitLength >> shift) & 0xffu));
    }

    std::array<std::uint32_t, 8> hash = {
        0x6a09e667u,
        0xbb67ae85u,
        0x3c6ef372u,
        0xa54ff53au,
        0x510e527fu,
        0x9b05688cu,
        0x1f83d9abu,
        0x5be0cd19u
    };

    for (std::size_t chunk = 0; chunk < bytes.size(); chunk += 64u) {
        std::array<std::uint32_t, 64> schedule{};
        for (std::size_t index = 0; index < 16u; ++index) {
            const auto offset = chunk + (index * 4u);
            schedule[index] =
                (static_cast<std::uint32_t>(bytes[offset]) << 24u) |
                (static_cast<std::uint32_t>(bytes[offset + 1u]) << 16u) |
                (static_cast<std::uint32_t>(bytes[offset + 2u]) << 8u) |
                static_cast<std::uint32_t>(bytes[offset + 3u]);
        }
        for (std::size_t index = 16u; index < 64u; ++index) {
            const auto s0 = rotateRight(schedule[index - 15u], 7u)
                ^ rotateRight(schedule[index - 15u], 18u)
                ^ (schedule[index - 15u] >> 3u);
            const auto s1 = rotateRight(schedule[index - 2u], 17u)
                ^ rotateRight(schedule[index - 2u], 19u)
                ^ (schedule[index - 2u] >> 10u);
            schedule[index] = schedule[index - 16u] + s0 + schedule[index - 7u] + s1;
        }

        auto a = hash[0];
        auto b = hash[1];
        auto c = hash[2];
        auto d = hash[3];
        auto e = hash[4];
        auto f = hash[5];
        auto g = hash[6];
        auto h = hash[7];

        for (std::size_t index = 0; index < 64u; ++index) {
            const auto sum1 = rotateRight(e, 6u) ^ rotateRight(e, 11u) ^ rotateRight(e, 25u);
            const auto choose = (e & f) ^ ((~e) & g);
            const auto temp1 = h + sum1 + choose + sha256RoundConstants[index] + schedule[index];
            const auto sum0 = rotateRight(a, 2u) ^ rotateRight(a, 13u) ^ rotateRight(a, 22u);
            const auto majority = (a & b) ^ (a & c) ^ (b & c);
            const auto temp2 = sum0 + majority;

            h = g;
            g = f;
            f = e;
            e = d + temp1;
            d = c;
            c = b;
            b = a;
            a = temp1 + temp2;
        }

        hash[0] += a;
        hash[1] += b;
        hash[2] += c;
        hash[3] += d;
        hash[4] += e;
        hash[5] += f;
        hash[6] += g;
        hash[7] += h;
    }

    std::ostringstream stream;
    stream << std::hex << std::setfill('0') << std::nouppercase;
    for (const auto value : hash) {
        stream << std::setw(8) << value;
    }
    return stream.str();
}

std::string manifestTemplateVersionString(ManifestTemplateVersion version)
{
    return to_string(version);
}

} // namespace

std::optional<ModuleRegistrationRecord> InMemoryModuleRegistrationStore::load(
    const std::string& moduleID) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = records_.find(moduleID);
    if (it == records_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::vector<ModuleRegistrationRecord> InMemoryModuleRegistrationStore::loadAll() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<ModuleRegistrationRecord> records;
    records.reserve(records_.size());
    for (const auto& [moduleID, record] : records_) {
        (void)moduleID;
        records.push_back(record);
    }
    return records;
}

void InMemoryModuleRegistrationStore::save(const ModuleRegistrationRecord& record)
{
    std::lock_guard<std::mutex> lock(mutex_);
    records_[record.moduleID] = record;
}

void InMemoryModuleRegistrationStore::remove(const std::string& moduleID)
{
    std::lock_guard<std::mutex> lock(mutex_);
    records_.erase(moduleID);
}

std::string SystemRegistrationClock::now() const
{
    const auto now = std::chrono::system_clock::now();
    const auto seconds = std::chrono::time_point_cast<std::chrono::seconds>(now);
    return std::to_string(seconds.time_since_epoch().count());
}

std::string Sha256ManifestDigestProvider::canonicalManifestJSON(const ModuleManifest& manifest) const
{
    const nlohmann::json manifestJSON = manifest;
    return canonicalJSON(manifestJSON);
}

std::string Sha256ManifestDigestProvider::digestManifest(const ModuleManifest& manifest) const
{
    return sha256Hex(canonicalManifestJSON(manifest));
}

ModuleRegistrationService::ModuleRegistrationService(
    std::shared_ptr<IModuleRegistrationStore> store,
    std::shared_ptr<IManifestDigestProvider> digestProvider,
    std::shared_ptr<IRegistrationClock> clock)
    : store_(std::move(store))
    , digestProvider_(std::move(digestProvider))
    , clock_(std::move(clock))
{
    if (!store_ || !digestProvider_ || !clock_) {
        throw std::invalid_argument("ModuleRegistrationService requires store, digest provider, and clock.");
    }
}

std::shared_ptr<ModuleRegistrationService> ModuleRegistrationService::makeInMemory()
{
    return std::make_shared<ModuleRegistrationService>(
        std::make_shared<InMemoryModuleRegistrationStore>(),
        std::make_shared<Sha256ManifestDigestProvider>(),
        std::make_shared<SystemRegistrationClock>());
}

ModuleRegistrationRecord ModuleRegistrationService::makeRecord(
    const ModuleManifest& manifest,
    const std::string& canonicalManifestHash,
    const std::string& registeredAt,
    const std::string& lastConfirmedAt,
    bool confirmed) const
{
    return ModuleRegistrationRecord{
        .moduleID = manifest.moduleID,
        .displayName = manifest.displayName,
        .moduleVersion = manifest.moduleVersion,
        .moduleType = manifest.moduleType,
        .entryPoint = manifest.entryPoint,
        .schemaVersion = manifest.schemaVersion,
        .manifestTemplateVersion = manifest.manifestTemplateVersion,
        .canonicalManifestHash = canonicalManifestHash,
        .supportedPlatforms = manifest.supportedPlatforms,
        .capabilitiesRequested = manifest.capabilitiesRequested,
        .defaultModuleRole = manifest.defaultModuleRole,
        .runtimeRequirements = manifest.runtimeRequirements,
        .registeredAt = registeredAt,
        .lastConfirmedAt = lastConfirmedAt,
        .confirmed = confirmed
    };
}

bool ModuleRegistrationService::recordMatchesManifest(
    const ModuleRegistrationRecord& record,
    const ModuleManifest& manifest,
    const std::string& canonicalManifestHash) const
{
    return record.moduleID == manifest.moduleID
        && record.displayName == manifest.displayName
        && record.moduleVersion == manifest.moduleVersion
        && record.moduleType == manifest.moduleType
        && record.entryPoint == manifest.entryPoint
        && record.schemaVersion == manifest.schemaVersion
        && record.manifestTemplateVersion == manifest.manifestTemplateVersion
        && record.canonicalManifestHash == canonicalManifestHash
        && record.supportedPlatforms == manifest.supportedPlatforms
        && record.capabilitiesRequested == manifest.capabilitiesRequested
        && record.defaultModuleRole == manifest.defaultModuleRole
        && record.runtimeRequirements == manifest.runtimeRequirements;
}

ModuleRegistrationRecord ModuleRegistrationService::registerDiscoveredManifest(
    const ModuleManifest& manifest)
{
    const auto now = clock_->now();
    const auto hash = digestProvider_->digestManifest(manifest);
    auto record = makeRecord(manifest, hash, now, "", false);
    store_->save(record);
    return record;
}

ModuleRegistrationRecord ModuleRegistrationService::confirmDiscoveredManifest(
    const ModuleManifest& manifest)
{
    const auto hash = digestProvider_->digestManifest(manifest);
    auto existing = store_->load(manifest.moduleID);
    if (!existing.has_value()) {
        existing = registerDiscoveredManifest(manifest);
    }

    if (!recordMatchesManifest(existing.value(), manifest, hash)) {
        throw std::runtime_error("Module registration record does not match discovered manifest: " + manifest.moduleID);
    }

    auto confirmed = existing.value();
    confirmed.lastConfirmedAt = clock_->now();
    confirmed.confirmed = true;
    store_->save(confirmed);
    return confirmed;
}

std::vector<ModuleRegistrationRecord> ModuleRegistrationService::confirmAllDiscoveredManifests(
    const std::vector<ModuleManifest>& manifests)
{
    std::vector<ModuleRegistrationRecord> confirmed;
    confirmed.reserve(manifests.size());
    std::vector<std::string> discoveredIDs;
    discoveredIDs.reserve(manifests.size());

    for (const auto& manifest : manifests) {
        confirmed.push_back(confirmDiscoveredManifest(manifest));
        discoveredIDs.push_back(manifest.moduleID);
    }

    reconcileRemovedManifests(discoveredIDs);
    return confirmed;
}

void ModuleRegistrationService::reconcileRemovedManifests(
    const std::vector<std::string>& discoveredModuleIDs)
{
    const std::set<std::string> discovered(discoveredModuleIDs.begin(), discoveredModuleIDs.end());
    for (const auto& record : store_->loadAll()) {
        if (!discovered.contains(record.moduleID)) {
            store_->remove(record.moduleID);
        }
    }
}

std::vector<ModuleRegistrationRecord> ModuleRegistrationService::registeredModules() const
{
    return store_->loadAll();
}

std::optional<ModuleRegistrationRecord> ModuleRegistrationService::load(
    const std::string& moduleID) const
{
    return store_->load(moduleID);
}

bool ModuleRegistrationService::isConfirmedMatch(const ModuleManifest& manifest) const
{
    const auto existing = store_->load(manifest.moduleID);
    if (!existing.has_value() || !existing->confirmed) {
        return false;
    }
    return recordMatchesManifest(
        existing.value(),
        manifest,
        digestProvider_->digestManifest(manifest));
}

void to_json(nlohmann::json& j, const ModuleRegistrationRecord& record)
{
    j = nlohmann::json{
        {"moduleID", record.moduleID},
        {"displayName", record.displayName},
        {"moduleVersion", record.moduleVersion},
        {"moduleType", record.moduleType},
        {"entryPoint", record.entryPoint},
        {"schemaVersion", record.schemaVersion},
        {"manifestTemplateVersion", manifestTemplateVersionString(record.manifestTemplateVersion)},
        {"canonicalManifestHash", record.canonicalManifestHash},
        {"supportedPlatforms", record.supportedPlatforms},
        {"capabilitiesRequested", record.capabilitiesRequested},
        {"runtimeRequirements", record.runtimeRequirements},
        {"registeredAt", record.registeredAt},
        {"lastConfirmedAt", record.lastConfirmedAt},
        {"confirmed", record.confirmed}
    };

    if (record.defaultModuleRole.has_value()) {
        j["defaultModuleRole"] = record.defaultModuleRole.value();
    } else {
        j["defaultModuleRole"] = nullptr;
    }
}

void from_json(const nlohmann::json& j, ModuleRegistrationRecord& record)
{
    j.at("moduleID").get_to(record.moduleID);
    j.at("displayName").get_to(record.displayName);
    j.at("moduleVersion").get_to(record.moduleVersion);
    j.at("moduleType").get_to(record.moduleType);
    j.at("entryPoint").get_to(record.entryPoint);
    j.at("schemaVersion").get_to(record.schemaVersion);
    j.at("manifestTemplateVersion").get_to(record.manifestTemplateVersion);
    j.at("canonicalManifestHash").get_to(record.canonicalManifestHash);
    j.at("supportedPlatforms").get_to(record.supportedPlatforms);
    j.at("capabilitiesRequested").get_to(record.capabilitiesRequested);
    if (j.contains("defaultModuleRole") && !j.at("defaultModuleRole").is_null()) {
        record.defaultModuleRole = j.at("defaultModuleRole").get<DefaultModuleRole>();
    } else {
        record.defaultModuleRole = std::nullopt;
    }
    j.at("runtimeRequirements").get_to(record.runtimeRequirements);
    j.at("registeredAt").get_to(record.registeredAt);
    j.at("lastConfirmedAt").get_to(record.lastConfirmedAt);
    j.at("confirmed").get_to(record.confirmed);
}

} // namespace Forsetti
