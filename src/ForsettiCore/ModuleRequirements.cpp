// Forsetti Framework for Windows
// Copyright (c) 2026 James Daley. All Rights Reserved.
// Proprietary and Confidential. Patent Pending.

#include "ForsettiCore/ModuleRequirements.h"
#include "ForsettiCore/ModuleModels.h"

#include <stdexcept>

namespace Forsetti {

std::string to_string(ManifestTemplateVersion version) {
    switch (version) {
        case ManifestTemplateVersion::V1_0: return "1.0";
        case ManifestTemplateVersion::V1_1: return "1.1";
    }
    throw std::invalid_argument("Unknown ManifestTemplateVersion value");
}

ManifestTemplateVersion manifestTemplateVersionFromString(const std::string& str) {
    if (str == "1.0") return ManifestTemplateVersion::V1_0;
    if (str == "1.1") return ManifestTemplateVersion::V1_1;
    throw std::invalid_argument("Unknown ManifestTemplateVersion string: " + str);
}

void to_json(nlohmann::json& j, ManifestTemplateVersion version) {
    j = to_string(version);
}

void from_json(const nlohmann::json& j, ManifestTemplateVersion& version) {
    version = manifestTemplateVersionFromString(j.get<std::string>());
}

std::string to_string(DefaultModuleRole role) {
    switch (role) {
        case DefaultModuleRole::UI:             return "ui";
        case DefaultModuleRole::SharedDatabase: return "shared_database";
        case DefaultModuleRole::Authentication: return "authentication";
        case DefaultModuleRole::Diagnostics:    return "diagnostics";
        case DefaultModuleRole::API:            return "api";
        case DefaultModuleRole::Security:       return "security";
    }
    throw std::invalid_argument("Unknown DefaultModuleRole value");
}

DefaultModuleRole defaultModuleRoleFromString(const std::string& str) {
    if (str == "ui")              return DefaultModuleRole::UI;
    if (str == "shared_database") return DefaultModuleRole::SharedDatabase;
    if (str == "authentication")  return DefaultModuleRole::Authentication;
    if (str == "diagnostics")     return DefaultModuleRole::Diagnostics;
    if (str == "api")             return DefaultModuleRole::API;
    if (str == "security")        return DefaultModuleRole::Security;
    throw std::invalid_argument("Unknown DefaultModuleRole string: " + str);
}

void to_json(nlohmann::json& j, DefaultModuleRole role) {
    j = to_string(role);
}

void from_json(const nlohmann::json& j, DefaultModuleRole& role) {
    role = defaultModuleRoleFromString(j.get<std::string>());
}

std::string to_string(ModuleIOKind kind) {
    switch (kind) {
        case ModuleIOKind::Networking:      return "networking";
        case ModuleIOKind::Storage:         return "storage";
        case ModuleIOKind::SecureStorage:   return "secure_storage";
        case ModuleIOKind::FileExport:      return "file_export";
        case ModuleIOKind::CryptoUtilities: return "crypto_utilities";
        case ModuleIOKind::Telemetry:       return "telemetry";
        case ModuleIOKind::SharedDatabase:  return "shared_database";
        case ModuleIOKind::Authentication:  return "authentication";
        case ModuleIOKind::Diagnostics:     return "diagnostics";
        case ModuleIOKind::API:             return "api";
        case ModuleIOKind::Security:        return "security";
    }
    throw std::invalid_argument("Unknown ModuleIOKind value");
}

ModuleIOKind moduleIOKindFromString(const std::string& str) {
    if (str == "networking")        return ModuleIOKind::Networking;
    if (str == "storage")           return ModuleIOKind::Storage;
    if (str == "secure_storage")    return ModuleIOKind::SecureStorage;
    if (str == "file_export")       return ModuleIOKind::FileExport;
    if (str == "crypto_utilities")  return ModuleIOKind::CryptoUtilities;
    if (str == "telemetry")         return ModuleIOKind::Telemetry;
    if (str == "shared_database")   return ModuleIOKind::SharedDatabase;
    if (str == "authentication")    return ModuleIOKind::Authentication;
    if (str == "diagnostics")       return ModuleIOKind::Diagnostics;
    if (str == "api")               return ModuleIOKind::API;
    if (str == "security")          return ModuleIOKind::Security;
    throw std::invalid_argument("Unknown ModuleIOKind string: " + str);
}

void to_json(nlohmann::json& j, ModuleIOKind kind) {
    j = to_string(kind);
}

void from_json(const nlohmann::json& j, ModuleIOKind& kind) {
    kind = moduleIOKindFromString(j.get<std::string>());
}

std::string to_string(ModuleIOAccess access) {
    switch (access) {
        case ModuleIOAccess::Read:      return "read";
        case ModuleIOAccess::Write:     return "write";
        case ModuleIOAccess::ReadWrite: return "read_write";
        case ModuleIOAccess::Execute:   return "execute";
        case ModuleIOAccess::Emit:      return "emit";
        case ModuleIOAccess::Consume:   return "consume";
    }
    throw std::invalid_argument("Unknown ModuleIOAccess value");
}

ModuleIOAccess moduleIOAccessFromString(const std::string& str) {
    if (str == "read")       return ModuleIOAccess::Read;
    if (str == "write")      return ModuleIOAccess::Write;
    if (str == "read_write") return ModuleIOAccess::ReadWrite;
    if (str == "execute")    return ModuleIOAccess::Execute;
    if (str == "emit")       return ModuleIOAccess::Emit;
    if (str == "consume")    return ModuleIOAccess::Consume;
    throw std::invalid_argument("Unknown ModuleIOAccess string: " + str);
}

void to_json(nlohmann::json& j, ModuleIOAccess access) {
    j = to_string(access);
}

void from_json(const nlohmann::json& j, ModuleIOAccess& access) {
    access = moduleIOAccessFromString(j.get<std::string>());
}

std::string to_string(ModuleDataIsolationMode mode) {
    switch (mode) {
        case ModuleDataIsolationMode::PrivateToModule:         return "private_to_module";
        case ModuleDataIsolationMode::FrameworkMediatedShared: return "framework_mediated_shared";
    }
    throw std::invalid_argument("Unknown ModuleDataIsolationMode value");
}

ModuleDataIsolationMode moduleDataIsolationModeFromString(const std::string& str) {
    if (str == "private_to_module")         return ModuleDataIsolationMode::PrivateToModule;
    if (str == "framework_mediated_shared") return ModuleDataIsolationMode::FrameworkMediatedShared;
    throw std::invalid_argument("Unknown ModuleDataIsolationMode string: " + str);
}

void to_json(nlohmann::json& j, ModuleDataIsolationMode mode) {
    j = to_string(mode);
}

void from_json(const nlohmann::json& j, ModuleDataIsolationMode& mode) {
    mode = moduleDataIsolationModeFromString(j.get<std::string>());
}

Capability capabilityForIOKind(ModuleIOKind kind) {
    switch (kind) {
        case ModuleIOKind::Networking:      return Capability::Networking;
        case ModuleIOKind::Storage:         return Capability::Storage;
        case ModuleIOKind::SecureStorage:   return Capability::SecureStorage;
        case ModuleIOKind::FileExport:      return Capability::FileExport;
        case ModuleIOKind::CryptoUtilities: return Capability::CryptoUtilities;
        case ModuleIOKind::Telemetry:       return Capability::Telemetry;
        case ModuleIOKind::SharedDatabase:  return Capability::SharedDatabase;
        case ModuleIOKind::Authentication:  return Capability::Authentication;
        case ModuleIOKind::Diagnostics:     return Capability::Diagnostics;
        case ModuleIOKind::API:             return Capability::API;
        case ModuleIOKind::Security:        return Capability::Security;
    }
    throw std::invalid_argument("Unknown ModuleIOKind value");
}

void to_json(nlohmann::json& j, const ModuleIORequirement& requirement) {
    j = nlohmann::json{
        {"requirementID", requirement.requirementID},
        {"kind",          requirement.kind},
        {"access",        requirement.access},
        {"required",      requirement.required}
    };

    if (requirement.description.has_value()) {
        j["description"] = requirement.description.value();
    }
}

void from_json(const nlohmann::json& j, ModuleIORequirement& requirement) {
    j.at("requirementID").get_to(requirement.requirementID);
    j.at("kind").get_to(requirement.kind);
    j.at("access").get_to(requirement.access);
    j.at("required").get_to(requirement.required);

    if (j.contains("description") && !j.at("description").is_null()) {
        requirement.description = j.at("description").get<std::string>();
    } else {
        requirement.description = std::nullopt;
    }
}

void to_json(nlohmann::json& j, const ModuleUIRequirements& requirements) {
    j = nlohmann::json{
        {"themeIDs",       requirements.themeIDs},
        {"viewIDs",        requirements.viewIDs},
        {"slotIDs",        requirements.slotIDs},
        {"toolbarItemIDs", requirements.toolbarItemIDs},
        {"routeIDs",       requirements.routeIDs},
        {"pointerIDs",     requirements.pointerIDs}
    };

    j["controlSchemeID"] = requirements.controlSchemeID.has_value()
        ? nlohmann::json(requirements.controlSchemeID.value())
        : nlohmann::json(nullptr);
    j["layoutID"] = requirements.layoutID.has_value()
        ? nlohmann::json(requirements.layoutID.value())
        : nlohmann::json(nullptr);
}

void from_json(const nlohmann::json& j, ModuleUIRequirements& requirements) {
    if (j.contains("controlSchemeID") && !j.at("controlSchemeID").is_null()) {
        requirements.controlSchemeID = j.at("controlSchemeID").get<std::string>();
    } else {
        requirements.controlSchemeID = std::nullopt;
    }

    if (j.contains("layoutID") && !j.at("layoutID").is_null()) {
        requirements.layoutID = j.at("layoutID").get<std::string>();
    } else {
        requirements.layoutID = std::nullopt;
    }

    j.at("themeIDs").get_to(requirements.themeIDs);
    j.at("viewIDs").get_to(requirements.viewIDs);
    j.at("slotIDs").get_to(requirements.slotIDs);
    j.at("toolbarItemIDs").get_to(requirements.toolbarItemIDs);
    j.at("routeIDs").get_to(requirements.routeIDs);
    j.at("pointerIDs").get_to(requirements.pointerIDs);
}

void to_json(nlohmann::json& j, const ModuleDataIsolation& isolation) {
    j = nlohmann::json{
        {"mode",                 isolation.mode},
        {"ownedStoreIDs",        isolation.ownedStoreIDs},
        {"requiredDefaultRoles", isolation.requiredDefaultRoles}
    };
}

void from_json(const nlohmann::json& j, ModuleDataIsolation& isolation) {
    j.at("mode").get_to(isolation.mode);
    j.at("ownedStoreIDs").get_to(isolation.ownedStoreIDs);
    j.at("requiredDefaultRoles").get_to(isolation.requiredDefaultRoles);
}

void to_json(nlohmann::json& j, const ModuleRuntimeRequirements& requirements) {
    j = nlohmann::json{
        {"io",            requirements.io},
        {"dataIsolation", requirements.dataIsolation}
    };

    if (requirements.ui.has_value()) {
        j["ui"] = requirements.ui.value();
    } else {
        j["ui"] = nullptr;
    }
}

void from_json(const nlohmann::json& j, ModuleRuntimeRequirements& requirements) {
    j.at("io").get_to(requirements.io);
    if (j.contains("ui") && !j.at("ui").is_null()) {
        requirements.ui = j.at("ui").get<ModuleUIRequirements>();
    } else {
        requirements.ui = std::nullopt;
    }
    j.at("dataIsolation").get_to(requirements.dataIsolation);
}

} // namespace Forsetti
