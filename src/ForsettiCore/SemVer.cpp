// Forsetti Framework for Windows
// Copyright (c) 2026 James Daley. All Rights Reserved.
// Proprietary and Confidential. Patent Pending.

#include "ForsettiCore/SemVer.h"
#include <algorithm>
#include <cctype>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

namespace Forsetti {

namespace {

bool isAsciiAlnumOrHyphen(char ch) {
    return std::isalnum(static_cast<unsigned char>(ch)) != 0 || ch == '-';
}

bool isNumericIdentifier(std::string_view value) {
    return !value.empty()
        && std::all_of(value.begin(), value.end(), [](char ch) {
            return std::isdigit(static_cast<unsigned char>(ch)) != 0;
        });
}

std::vector<std::string> split(const std::string& value, char delimiter) {
    std::vector<std::string> parts;
    std::string current;
    for (char ch : value) {
        if (ch == delimiter) {
            parts.push_back(current);
            current.clear();
        } else {
            current.push_back(ch);
        }
    }
    parts.push_back(current);
    return parts;
}

bool isValidPrerelease(const std::string& prerelease) {
    if (prerelease.empty()) {
        return false;
    }

    for (const auto& identifier : split(prerelease, '.')) {
        if (identifier.empty()) {
            return false;
        }
        if (!std::all_of(identifier.begin(), identifier.end(), isAsciiAlnumOrHyphen)) {
            return false;
        }
        if (isNumericIdentifier(identifier) && identifier.size() > 1 && identifier.front() == '0') {
            return false;
        }
    }

    return true;
}

std::optional<int> parseVersionComponent(const std::string& value) {
    if (value.empty()) {
        return std::nullopt;
    }
    if (value.size() > 1 && value.front() == '0') {
        return std::nullopt;
    }
    if (!std::all_of(value.begin(), value.end(), [](char ch) {
        return std::isdigit(static_cast<unsigned char>(ch)) != 0;
    })) {
        return std::nullopt;
    }

    long long parsed = 0;
    for (char ch : value) {
        parsed = (parsed * 10) + (ch - '0');
        if (parsed > std::numeric_limits<int>::max()) {
            return std::nullopt;
        }
    }

    return static_cast<int>(parsed);
}

std::strong_ordering comparePrereleaseIdentifier(
    const std::string& left,
    const std::string& right)
{
    const bool leftNumeric = isNumericIdentifier(left);
    const bool rightNumeric = isNumericIdentifier(right);

    if (leftNumeric && rightNumeric) {
        if (left.size() < right.size()) return std::strong_ordering::less;
        if (left.size() > right.size()) return std::strong_ordering::greater;
        if (left < right) return std::strong_ordering::less;
        if (left > right) return std::strong_ordering::greater;
        return std::strong_ordering::equal;
    }

    if (leftNumeric && !rightNumeric) {
        return std::strong_ordering::less;
    }
    if (!leftNumeric && rightNumeric) {
        return std::strong_ordering::greater;
    }

    if (left < right) return std::strong_ordering::less;
    if (left > right) return std::strong_ordering::greater;
    return std::strong_ordering::equal;
}

void validateSemVerComponents(int major, int minor, int patch, const std::optional<std::string>& prerelease) {
    if (major < 0 || minor < 0 || patch < 0) {
        throw std::invalid_argument("Semantic version components must be nonnegative.");
    }
    if (prerelease.has_value() && !isValidPrerelease(prerelease.value())) {
        throw std::invalid_argument("Semantic version prerelease value is invalid.");
    }
}

} // namespace

SemVer::SemVer(int major, int minor, int patch, std::optional<std::string> prerelease)
    : major(major), minor(minor), patch(patch), prerelease(std::move(prerelease))
{
    validateSemVerComponents(this->major, this->minor, this->patch, this->prerelease);
}

std::optional<SemVer> SemVer::fromString(const std::string& versionString) {
    if (versionString.empty()) {
        return std::nullopt;
    }
    if (versionString.find('+') != std::string::npos) {
        return std::nullopt;
    }
    if (std::any_of(versionString.begin(), versionString.end(), [](char ch) {
        return std::isspace(static_cast<unsigned char>(ch)) != 0;
    })) {
        return std::nullopt;
    }

    std::string core = versionString;
    std::optional<std::string> prereleaseValue;
    const auto hyphenPos = versionString.find('-');
    if (hyphenPos != std::string::npos) {
        core = versionString.substr(0, hyphenPos);
        prereleaseValue = versionString.substr(hyphenPos + 1);
        if (!isValidPrerelease(prereleaseValue.value())) {
            return std::nullopt;
        }
    }

    const auto components = split(core, '.');
    if (components.size() != 3) {
        return std::nullopt;
    }

    auto majorValue = parseVersionComponent(components[0]);
    auto minorValue = parseVersionComponent(components[1]);
    auto patchValue = parseVersionComponent(components[2]);
    if (!majorValue.has_value() || !minorValue.has_value() || !patchValue.has_value()) {
        return std::nullopt;
    }

    try {
        return SemVer{majorValue.value(), minorValue.value(), patchValue.value(), prereleaseValue};
    } catch (const std::invalid_argument&) {
        return std::nullopt;
    }
}

std::string SemVer::toString() const {
    std::string result = std::to_string(major) + "." +
                         std::to_string(minor) + "." +
                         std::to_string(patch);
    if (prerelease.has_value()) {
        result += "-" + prerelease.value();
    }
    return result;
}

std::strong_ordering SemVer::operator<=>(const SemVer& other) const {
    // Compare major.minor.patch first
    if (auto cmp = major <=> other.major; cmp != 0) return cmp;
    if (auto cmp = minor <=> other.minor; cmp != 0) return cmp;
    if (auto cmp = patch <=> other.patch; cmp != 0) return cmp;

    const bool hasPreA = prerelease.has_value();
    const bool hasPreB = other.prerelease.has_value();

    if (!hasPreA && !hasPreB) return std::strong_ordering::equal;
    if (!hasPreA && hasPreB) return std::strong_ordering::greater;  // release > prerelease
    if (hasPreA && !hasPreB) return std::strong_ordering::less;     // prerelease < release

    const auto leftParts = split(prerelease.value(), '.');
    const auto rightParts = split(other.prerelease.value(), '.');
    const auto sharedSize = std::min(leftParts.size(), rightParts.size());
    for (std::size_t index = 0; index < sharedSize; ++index) {
        if (auto cmp = comparePrereleaseIdentifier(leftParts[index], rightParts[index]); cmp != 0) {
            return cmp;
        }
    }
    if (leftParts.size() < rightParts.size()) return std::strong_ordering::less;
    if (leftParts.size() > rightParts.size()) return std::strong_ordering::greater;
    return std::strong_ordering::equal;
}

// JSON serialization
void to_json(nlohmann::json& j, const SemVer& v) {
    j = nlohmann::json{
        {"major", v.major},
        {"minor", v.minor},
        {"patch", v.patch}
    };
    if (v.prerelease.has_value()) {
        j["prerelease"] = v.prerelease.value();
    } else {
        j["prerelease"] = nullptr;
    }
}

void from_json(const nlohmann::json& j, SemVer& v) {
    const auto major = j.at("major").get<int>();
    const auto minor = j.at("minor").get<int>();
    const auto patch = j.at("patch").get<int>();

    std::optional<std::string> prerelease;
    if (j.contains("prerelease") && !j["prerelease"].is_null()) {
        prerelease = j["prerelease"].get<std::string>();
    }

    v = SemVer{major, minor, patch, prerelease};
}

} // namespace Forsetti
