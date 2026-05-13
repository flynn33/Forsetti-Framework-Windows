// Forsetti Framework for Windows
// Copyright (c) 2026 James Daley. All Rights Reserved.
// Proprietary and Confidential. Patent Pending.

#include "ForsettiCore/ManifestLoader.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <set>

namespace Forsetti {

namespace {

[[noreturn]] void throwInvalidManifest(const std::filesystem::path& path, const std::string& reason) {
    throw ManifestLoaderException(
        ManifestLoaderError::InvalidManifest,
        "Invalid manifest in file: " + path.string() + ": " + reason
    );
}

bool isBlank(const std::string& value) {
    return std::all_of(value.begin(), value.end(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    });
}

bool isAlphaNumeric(char ch) {
    return std::isalnum(static_cast<unsigned char>(ch)) != 0;
}

bool isSafeModuleID(const std::string& value) {
    if (value.empty() || isBlank(value) || value.front() == '.' || value.back() == '.') {
        return false;
    }

    bool sawDot = false;
    bool segmentHasCharacter = false;
    char previous = 0;

    for (char ch : value) {
        if (ch == '.') {
            if (!segmentHasCharacter || previous == '-') {
                return false;
            }
            sawDot = true;
            segmentHasCharacter = false;
        } else if (isAlphaNumeric(ch) || ch == '-') {
            if (!segmentHasCharacter && ch == '-') {
                return false;
            }
            segmentHasCharacter = true;
        } else {
            return false;
        }

        previous = ch;
    }

    return sawDot && segmentHasCharacter && previous != '-';
}

bool isSafeEntryPoint(const std::string& value) {
    if (value.empty() || isBlank(value)) {
        return false;
    }

    const char first = value.front();
    if (!(std::isalpha(static_cast<unsigned char>(first)) != 0 || first == '_')) {
        return false;
    }

    return std::all_of(value.begin() + 1, value.end(), [](char ch) {
        return std::isalnum(static_cast<unsigned char>(ch)) != 0
            || ch == '_'
            || ch == ':'
            || ch == '.';
    });
}

bool hasNegativeComponent(const SemVer& version) {
    return version.major < 0 || version.minor < 0 || version.patch < 0;
}

void requireStringField(const nlohmann::json& j, const char* key, const std::filesystem::path& path) {
    if (!j.contains(key) || !j.at(key).is_string()) {
        throwInvalidManifest(path, std::string("missing or invalid string field: ") + key);
    }
}

void requireObjectField(const nlohmann::json& j, const char* key, const std::filesystem::path& path) {
    if (!j.contains(key) || !j.at(key).is_object()) {
        throwInvalidManifest(path, std::string("missing or invalid object field: ") + key);
    }
}

void requireArrayField(const nlohmann::json& j, const char* key, const std::filesystem::path& path) {
    if (!j.contains(key) || !j.at(key).is_array()) {
        throwInvalidManifest(path, std::string("missing or invalid array field: ") + key);
    }
}

void validateManifestJSONShape(const nlohmann::json& j, const std::filesystem::path& path) {
    requireStringField(j, "schemaVersion", path);
    requireStringField(j, "moduleID", path);
    requireStringField(j, "displayName", path);
    requireObjectField(j, "moduleVersion", path);
    requireStringField(j, "moduleType", path);
    requireArrayField(j, "supportedPlatforms", path);
    requireObjectField(j, "minForsettiVersion", path);
    requireArrayField(j, "capabilitiesRequested", path);
    requireStringField(j, "entryPoint", path);
}

void validateManifest(const ModuleManifest& manifest, const std::filesystem::path& path) {
    if (manifest.schemaVersion != "1.0") {
        throwInvalidManifest(path, "unsupported schemaVersion: " + manifest.schemaVersion);
    }

    if (!isSafeModuleID(manifest.moduleID)) {
        throwInvalidManifest(path, "moduleID is blank or unsafe: " + manifest.moduleID);
    }

    if (manifest.displayName.empty() || isBlank(manifest.displayName)) {
        throwInvalidManifest(path, "displayName is blank");
    }

    if (!isSafeEntryPoint(manifest.entryPoint)) {
        throwInvalidManifest(path, "entryPoint is blank or unsafe: " + manifest.entryPoint);
    }

    if (manifest.supportedPlatforms.empty()) {
        throwInvalidManifest(path, "supportedPlatforms must not be empty");
    }

    if (hasNegativeComponent(manifest.moduleVersion)) {
        throwInvalidManifest(path, "moduleVersion contains a negative component");
    }

    if (hasNegativeComponent(manifest.minForsettiVersion)) {
        throwInvalidManifest(path, "minForsettiVersion contains a negative component");
    }

    if (manifest.maxForsettiVersion.has_value()) {
        if (hasNegativeComponent(manifest.maxForsettiVersion.value())) {
            throwInvalidManifest(path, "maxForsettiVersion contains a negative component");
        }

        if (manifest.maxForsettiVersion.value() < manifest.minForsettiVersion) {
            throwInvalidManifest(path, "maxForsettiVersion is lower than minForsettiVersion");
        }
    }

    if (manifest.iapProductID.has_value() && !isSafeModuleID(manifest.iapProductID.value())) {
        throwInvalidManifest(path, "iapProductID is blank or unsafe: " + manifest.iapProductID.value());
    }
}

} // namespace

// MARK: - ManifestLoader

std::vector<ModuleManifest> ManifestLoader::loadManifests(const std::string& directoryPath) {
    namespace fs = std::filesystem;

    // Verify the directory exists
    if (!fs::exists(directoryPath) || !fs::is_directory(directoryPath)) {
        throw ManifestLoaderException(
            ManifestLoaderError::DirectoryNotFound,
            "Directory not found: " + directoryPath
        );
    }

    std::vector<ModuleManifest> manifests;
    std::set<std::string> seenModuleIDs;

    for (const auto& entry : fs::recursive_directory_iterator(directoryPath)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        if (entry.path().extension() != ".json") {
            continue;
        }

        // Read the file contents
        std::ifstream file(entry.path());
        if (!file.is_open()) {
            continue; // Silently skip files we cannot open
        }

        nlohmann::json j;
        try {
            j = nlohmann::json::parse(file);
        } catch (const nlohmann::json::parse_error&) {
            // Silently skip files with invalid JSON
            continue;
        }

        // Check if this JSON looks like a manifest
        if (!looksLikeManifestJSON(j)) {
            // Silently skip non-manifest JSON files
            continue;
        }

        validateManifestJSONShape(j, entry.path());

        // Parse the manifest
        ModuleManifest manifest;
        try {
            manifest = j.get<ModuleManifest>();
        } catch (const std::exception& ex) {
            throwInvalidManifest(entry.path(), ex.what());
        }

        validateManifest(manifest, entry.path());

        // Check for duplicate module IDs
        if (seenModuleIDs.count(manifest.moduleID) > 0) {
            throw ManifestLoaderException(
                ManifestLoaderError::DuplicateModuleID,
                "Duplicate module ID: " + manifest.moduleID
            );
        }

        seenModuleIDs.insert(manifest.moduleID);
        manifests.push_back(std::move(manifest));
    }

    return manifests;
}

bool ManifestLoader::looksLikeManifestJSON(const nlohmann::json& j) {
    if (!j.is_object()) {
        return false;
    }

    return j.contains("schemaVersion")
        && j.contains("moduleID")
        && j.contains("displayName");
}

} // namespace Forsetti
