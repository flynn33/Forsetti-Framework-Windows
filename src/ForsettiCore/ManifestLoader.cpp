// Forsetti Framework for Windows
// Copyright (c) 2026 James Daley. All Rights Reserved.
// Proprietary and Confidential. Patent Pending.

#include "ForsettiCore/ManifestLoader.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <set>
#include <string_view>

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

bool isSafeDeclaredID(const std::string& value) {
    if (value.empty() || isBlank(value)) {
        return false;
    }

    if (!isAlphaNumeric(value.front())) {
        return false;
    }

    return std::all_of(value.begin() + 1, value.end(), [](char ch) {
        return std::isalnum(static_cast<unsigned char>(ch)) != 0
            || ch == '_'
            || ch == '.'
            || ch == '-';
    });
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

void requireNullableObjectField(const nlohmann::json& j, const char* key, const std::filesystem::path& path) {
    if (!j.contains(key) || !(j.at(key).is_object() || j.at(key).is_null())) {
        throwInvalidManifest(path, std::string("missing or invalid object/null field: ") + key);
    }
}

void requireNullableStringField(const nlohmann::json& j, const char* key, const std::filesystem::path& path) {
    if (!j.contains(key) || !(j.at(key).is_string() || j.at(key).is_null())) {
        throwInvalidManifest(path, std::string("missing or invalid string/null field: ") + key);
    }
}

void validateKnownKeys(
    const nlohmann::json& j,
    const std::set<std::string>& allowedKeys,
    const std::filesystem::path& path,
    std::string_view scope)
{
    for (const auto& [key, value] : j.items()) {
        (void)value;
        if (!allowedKeys.contains(key)) {
            throwInvalidManifest(path, std::string(scope) + " contains unsupported field: " + key);
        }
    }
}

void requireRuntimeRequirementsShape(const nlohmann::json& j, const std::filesystem::path& path) {
    validateKnownKeys(j, {"io", "ui", "dataIsolation"}, path, "runtimeRequirements");
    requireArrayField(j, "io", path);
    if (!j.contains("ui") || !(j.at("ui").is_object() || j.at("ui").is_null())) {
        throwInvalidManifest(path, "runtimeRequirements.ui must be an object or null");
    }
    requireObjectField(j, "dataIsolation", path);

    for (const auto& ioRequirement : j.at("io")) {
        if (!ioRequirement.is_object()) {
            throwInvalidManifest(path, "runtimeRequirements.io entries must be objects");
        }
        validateKnownKeys(
            ioRequirement,
            {"requirementID", "kind", "access", "required", "description"},
            path,
            "runtimeRequirements.io entry");
        requireStringField(ioRequirement, "requirementID", path);
        requireStringField(ioRequirement, "kind", path);
        requireStringField(ioRequirement, "access", path);
        if (!ioRequirement.contains("required") || !ioRequirement.at("required").is_boolean()) {
            throwInvalidManifest(path, "runtimeRequirements.io.required must be a boolean");
        }
        if (ioRequirement.contains("description") && !ioRequirement.at("description").is_string()) {
            throwInvalidManifest(path, "runtimeRequirements.io.description must be a string");
        }
    }

    if (j.at("ui").is_object()) {
        const auto& ui = j.at("ui");
        validateKnownKeys(
            ui,
            {"controlSchemeID", "layoutID", "themeIDs", "viewIDs", "slotIDs", "toolbarItemIDs", "routeIDs", "pointerIDs"},
            path,
            "runtimeRequirements.ui");
        if (ui.contains("controlSchemeID") && !(ui.at("controlSchemeID").is_string() || ui.at("controlSchemeID").is_null())) {
            throwInvalidManifest(path, "runtimeRequirements.ui.controlSchemeID must be a string or null");
        }
        if (ui.contains("layoutID") && !(ui.at("layoutID").is_string() || ui.at("layoutID").is_null())) {
            throwInvalidManifest(path, "runtimeRequirements.ui.layoutID must be a string or null");
        }
        requireArrayField(ui, "themeIDs", path);
        requireArrayField(ui, "viewIDs", path);
        requireArrayField(ui, "slotIDs", path);
        requireArrayField(ui, "toolbarItemIDs", path);
        requireArrayField(ui, "routeIDs", path);
        requireArrayField(ui, "pointerIDs", path);
    }

    const auto& dataIsolation = j.at("dataIsolation");
    validateKnownKeys(
        dataIsolation,
        {"mode", "ownedStoreIDs", "requiredDefaultRoles"},
        path,
        "runtimeRequirements.dataIsolation");
    requireStringField(dataIsolation, "mode", path);
    requireArrayField(dataIsolation, "ownedStoreIDs", path);
    requireArrayField(dataIsolation, "requiredDefaultRoles", path);
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

    const auto schemaVersion = j.at("schemaVersion").get<std::string>();
    if (schemaVersion != "1.0" && schemaVersion != "1.1") {
        throwInvalidManifest(path, "unsupported schemaVersion: " + schemaVersion);
    }

    validateKnownKeys(
        j,
        {
            "schemaVersion",
            "manifestTemplateVersion",
            "moduleID",
            "displayName",
            "moduleVersion",
            "moduleType",
            "supportedPlatforms",
            "minForsettiVersion",
            "maxForsettiVersion",
            "capabilitiesRequested",
            "iapProductID",
            "entryPoint",
            "defaultModuleRole",
            "runtimeRequirements"
        },
        path,
        "manifest");

    if (schemaVersion == "1.1") {
        requireStringField(j, "manifestTemplateVersion", path);
        requireNullableObjectField(j, "maxForsettiVersion", path);
        requireNullableStringField(j, "iapProductID", path);
        requireNullableStringField(j, "defaultModuleRole", path);
        requireObjectField(j, "runtimeRequirements", path);
        requireRuntimeRequirementsShape(j.at("runtimeRequirements"), path);
    } else {
        if (j.contains("manifestTemplateVersion") && !j.at("manifestTemplateVersion").is_string()) {
            throwInvalidManifest(path, "manifestTemplateVersion must be a string when present");
        }
        if (j.contains("runtimeRequirements") && !j.at("runtimeRequirements").is_object()) {
            throwInvalidManifest(path, "runtimeRequirements must be an object when present");
        }
    }
}

template<typename T, typename ToString>
void validateUniqueEnumValues(
    const std::vector<T>& values,
    ToString toString,
    const std::filesystem::path& path,
    const std::string& field)
{
    std::set<std::string> seen;
    for (const auto& value : values) {
        const auto key = toString(value);
        if (!seen.insert(key).second) {
            throwInvalidManifest(path, field + " contains duplicate value: " + key);
        }
    }
}

void validateUniqueIDs(
    const std::vector<std::string>& ids,
    const std::filesystem::path& path,
    const std::string& field)
{
    std::set<std::string> seen;
    for (const auto& id : ids) {
        if (!isSafeDeclaredID(id)) {
            throwInvalidManifest(path, field + " contains blank or unsafe ID: " + id);
        }
        if (!seen.insert(id).second) {
            throwInvalidManifest(path, field + " contains duplicate ID: " + id);
        }
    }
}

bool hasCapability(const ModuleManifest& manifest, Capability capability) {
    return std::find(
        manifest.capabilitiesRequested.begin(),
        manifest.capabilitiesRequested.end(),
        capability) != manifest.capabilitiesRequested.end();
}

bool hasIOKind(const ModuleManifest& manifest, ModuleIOKind kind) {
    return std::any_of(
        manifest.runtimeRequirements.io.begin(),
        manifest.runtimeRequirements.io.end(),
        [kind](const ModuleIORequirement& requirement) {
            return requirement.kind == kind;
        });
}

bool requiresDefaultRole(const ModuleManifest& manifest, DefaultModuleRole role) {
    return std::find(
        manifest.runtimeRequirements.dataIsolation.requiredDefaultRoles.begin(),
        manifest.runtimeRequirements.dataIsolation.requiredDefaultRoles.end(),
        role) != manifest.runtimeRequirements.dataIsolation.requiredDefaultRoles.end();
}

void validateRoleCompatibility(const ModuleManifest& manifest, const std::filesystem::path& path) {
    if (!manifest.defaultModuleRole.has_value()) {
        return;
    }

    const auto role = manifest.defaultModuleRole.value();
    if (role == DefaultModuleRole::UI) {
        if (manifest.moduleType != ModuleType::UI && manifest.moduleType != ModuleType::App) {
            throwInvalidManifest(path, "defaultModuleRole ui requires moduleType ui or app");
        }
        return;
    }

    if (manifest.moduleType != ModuleType::Service) {
        throwInvalidManifest(
            path,
            "defaultModuleRole " + to_string(role) + " requires moduleType service");
    }
}

void validateUIRequirements(const ModuleManifest& manifest, const std::filesystem::path& path) {
    const auto hasUI = manifest.runtimeRequirements.ui.has_value();
    if (manifest.moduleType == ModuleType::Service && hasUI) {
        throwInvalidManifest(path, "service modules must not declare runtimeRequirements.ui");
    }

    if (manifest.schemaVersion == "1.1" &&
        (manifest.moduleType == ModuleType::UI || manifest.moduleType == ModuleType::App) &&
        !hasUI) {
        throwInvalidManifest(path, "ui and app modules must declare runtimeRequirements.ui");
    }

    if (!hasUI) {
        return;
    }

    const auto& ui = manifest.runtimeRequirements.ui.value();
    std::set<std::string> allUIIDs;
    const auto registerID = [&](const std::optional<std::string>& id, const std::string& field) {
        if (!id.has_value()) {
            return;
        }
        if (!isSafeDeclaredID(id.value())) {
            throwInvalidManifest(path, field + " contains blank or unsafe ID: " + id.value());
        }
        if (!allUIIDs.insert(id.value()).second) {
            throwInvalidManifest(path, "runtimeRequirements.ui contains duplicate ID: " + id.value());
        }
    };
    const auto registerIDs = [&](const std::vector<std::string>& ids, const std::string& field) {
        validateUniqueIDs(ids, path, field);
        for (const auto& id : ids) {
            if (!allUIIDs.insert(id).second) {
                throwInvalidManifest(path, "runtimeRequirements.ui contains duplicate ID: " + id);
            }
        }
    };

    registerID(ui.controlSchemeID, "runtimeRequirements.ui.controlSchemeID");
    registerID(ui.layoutID, "runtimeRequirements.ui.layoutID");
    registerIDs(ui.themeIDs, "runtimeRequirements.ui.themeIDs");
    registerIDs(ui.viewIDs, "runtimeRequirements.ui.viewIDs");
    registerIDs(ui.slotIDs, "runtimeRequirements.ui.slotIDs");
    registerIDs(ui.toolbarItemIDs, "runtimeRequirements.ui.toolbarItemIDs");
    registerIDs(ui.routeIDs, "runtimeRequirements.ui.routeIDs");
    registerIDs(ui.pointerIDs, "runtimeRequirements.ui.pointerIDs");
}

void validateRuntimeRequirements(const ModuleManifest& manifest, const std::filesystem::path& path) {
    std::set<std::string> ioIDs;
    for (const auto& requirement : manifest.runtimeRequirements.io) {
        if (!isSafeDeclaredID(requirement.requirementID)) {
            throwInvalidManifest(
                path,
                "runtimeRequirements.io.requirementID is blank or unsafe: " + requirement.requirementID);
        }
        if (!ioIDs.insert(requirement.requirementID).second) {
            throwInvalidManifest(
                path,
                "runtimeRequirements.io contains duplicate requirementID: " + requirement.requirementID);
        }

        const auto requiredCapability = capabilityForIOKind(requirement.kind);
        if (!hasCapability(manifest, requiredCapability)) {
            throwInvalidManifest(
                path,
                "runtimeRequirements.io requirement '" + requirement.requirementID
                    + "' requires capability " + to_string(requiredCapability));
        }
    }

    validateUniqueIDs(
        manifest.runtimeRequirements.dataIsolation.ownedStoreIDs,
        path,
        "runtimeRequirements.dataIsolation.ownedStoreIDs");
    validateUniqueEnumValues(
        manifest.runtimeRequirements.dataIsolation.requiredDefaultRoles,
        [](DefaultModuleRole role) { return to_string(role); },
        path,
        "runtimeRequirements.dataIsolation.requiredDefaultRoles");

    if (manifest.runtimeRequirements.dataIsolation.mode == ModuleDataIsolationMode::FrameworkMediatedShared) {
        const bool declaresSharedDatabase =
            manifest.defaultModuleRole == DefaultModuleRole::SharedDatabase
            || requiresDefaultRole(manifest, DefaultModuleRole::SharedDatabase)
            || hasIOKind(manifest, ModuleIOKind::SharedDatabase);

        if (!declaresSharedDatabase) {
            throwInvalidManifest(
                path,
                "framework_mediated_shared data isolation requires a shared_database role or I/O requirement");
        }
    }
}

void validateManifest(const ModuleManifest& manifest, const std::filesystem::path& path) {
    if (!manifest.isSchemaValid()) {
        throwInvalidManifest(path, "unsupported schemaVersion: " + manifest.schemaVersion);
    }

    if (manifest.schemaVersion == "1.0" &&
        manifest.manifestTemplateVersion != ManifestTemplateVersion::V1_0) {
        throwInvalidManifest(path, "schemaVersion 1.0 requires manifestTemplateVersion 1.0");
    }

    if (manifest.schemaVersion == "1.1" &&
        manifest.manifestTemplateVersion != ManifestTemplateVersion::V1_1) {
        throwInvalidManifest(path, "schemaVersion 1.1 requires manifestTemplateVersion 1.1");
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

    validateUniqueEnumValues(
        manifest.supportedPlatforms,
        [](Platform platform) { return to_string(platform); },
        path,
        "supportedPlatforms");
    validateUniqueEnumValues(
        manifest.capabilitiesRequested,
        [](Capability capability) { return to_string(capability); },
        path,
        "capabilitiesRequested");

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

    validateRoleCompatibility(manifest, path);
    validateUIRequirements(manifest, path);
    validateRuntimeRequirements(manifest, path);
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
