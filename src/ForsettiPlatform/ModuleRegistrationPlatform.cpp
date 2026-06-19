// Forsetti Framework for Windows
// Copyright (c) 2026 James Daley. All Rights Reserved.
// Proprietary and Confidential. Patent Pending.

#include "ForsettiPlatform/ModuleRegistrationPlatform.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <bcrypt.h>

#include <iomanip>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <cstdint>
#include <utility>

namespace Forsetti {

namespace {

constexpr wchar_t RegistrationRoot[] = L"Software\\Forsetti\\Framework\\ModuleRegistrations";

std::wstring utf8ToWideRegistration(const std::string& value)
{
    if (value.empty()) {
        return {};
    }

    const int size = MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        value.data(),
        static_cast<int>(value.size()),
        nullptr,
        0);
    if (size <= 0) {
        throw std::invalid_argument("Invalid UTF-8 string.");
    }

    std::wstring result(static_cast<std::size_t>(size), L'\0');
    MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        value.data(),
        static_cast<int>(value.size()),
        result.data(),
        size);
    return result;
}

std::string wideToUtf8Registration(const std::wstring& value)
{
    if (value.empty()) {
        return {};
    }

    const int size = WideCharToMultiByte(
        CP_UTF8,
        WC_ERR_INVALID_CHARS,
        value.data(),
        static_cast<int>(value.size()),
        nullptr,
        0,
        nullptr,
        nullptr);
    if (size <= 0) {
        throw std::runtime_error("Failed to encode UTF-8 string.");
    }

    std::string result(static_cast<std::size_t>(size), '\0');
    WideCharToMultiByte(
        CP_UTF8,
        WC_ERR_INVALID_CHARS,
        value.data(),
        static_cast<int>(value.size()),
        result.data(),
        size,
        nullptr,
        nullptr);
    return result;
}

[[noreturn]] void throwWin32RegistrationError(const std::string& operation, DWORD error)
{
    throw std::runtime_error(operation + " failed with Windows error " + std::to_string(error));
}

class RegistrationRegistryKey final {
public:
    RegistrationRegistryKey(const std::wstring& subkey, REGSAM access)
    {
        const auto status = RegCreateKeyExW(
            HKEY_CURRENT_USER,
            subkey.c_str(),
            0,
            nullptr,
            REG_OPTION_NON_VOLATILE,
            access,
            nullptr,
            &key_,
            nullptr);
        if (status != ERROR_SUCCESS) {
            throwWin32RegistrationError("RegCreateKeyExW", status);
        }
    }

    RegistrationRegistryKey(const RegistrationRegistryKey&) = delete;
    RegistrationRegistryKey& operator=(const RegistrationRegistryKey&) = delete;

    ~RegistrationRegistryKey()
    {
        if (key_) {
            RegCloseKey(key_);
        }
    }

    HKEY get() const noexcept { return key_; }

private:
    HKEY key_ = nullptr;
};

class BCryptAlgorithmHandle final {
public:
    BCryptAlgorithmHandle()
    {
        const auto status = BCryptOpenAlgorithmProvider(
            &handle_,
            BCRYPT_SHA256_ALGORITHM,
            nullptr,
            0);
        if (status < 0) {
            throw std::runtime_error("BCryptOpenAlgorithmProvider failed.");
        }
    }

    BCryptAlgorithmHandle(const BCryptAlgorithmHandle&) = delete;
    BCryptAlgorithmHandle& operator=(const BCryptAlgorithmHandle&) = delete;

    ~BCryptAlgorithmHandle()
    {
        if (handle_) {
            BCryptCloseAlgorithmProvider(handle_, 0);
        }
    }

    BCRYPT_ALG_HANDLE get() const noexcept { return handle_; }

private:
    BCRYPT_ALG_HANDLE handle_ = nullptr;
};

std::string bytesToLowerHex(const std::vector<std::uint8_t>& bytes)
{
    std::ostringstream stream;
    stream << std::hex << std::setfill('0') << std::nouppercase;
    for (const auto byte : bytes) {
        stream << std::setw(2) << static_cast<int>(byte);
    }
    return stream.str();
}

} // namespace

RegistryModuleRegistrationStore::RegistryModuleRegistrationStore()
    : rootSubkey_(RegistrationRoot)
{
}

RegistryModuleRegistrationStore::RegistryModuleRegistrationStore(std::wstring rootSubkey)
    : rootSubkey_(std::move(rootSubkey))
{
}

std::optional<ModuleRegistrationRecord> RegistryModuleRegistrationStore::load(
    const std::string& moduleID) const
{
    const auto valueName = utf8ToWideRegistration(moduleID);

    std::lock_guard<std::mutex> lock(mutex_);
    RegistrationRegistryKey key(rootSubkey_, KEY_QUERY_VALUE);

    DWORD type = 0;
    DWORD byteCount = 0;
    auto status = RegQueryValueExW(
        key.get(),
        valueName.c_str(),
        nullptr,
        &type,
        nullptr,
        &byteCount);
    if (status == ERROR_FILE_NOT_FOUND) {
        return std::nullopt;
    }
    if (status != ERROR_SUCCESS) {
        throwWin32RegistrationError("RegQueryValueExW", status);
    }
    if (type != REG_SZ && type != REG_EXPAND_SZ) {
        throw std::runtime_error("Module registration value is not a string.");
    }

    std::wstring buffer(byteCount / sizeof(wchar_t), L'\0');
    status = RegQueryValueExW(
        key.get(),
        valueName.c_str(),
        nullptr,
        &type,
        reinterpret_cast<BYTE*>(buffer.data()),
        &byteCount);
    if (status != ERROR_SUCCESS) {
        throwWin32RegistrationError("RegQueryValueExW", status);
    }

    if (!buffer.empty() && buffer.back() == L'\0') {
        buffer.pop_back();
    }

    return nlohmann::json::parse(wideToUtf8Registration(buffer)).get<ModuleRegistrationRecord>();
}

std::vector<ModuleRegistrationRecord> RegistryModuleRegistrationStore::loadAll() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    RegistrationRegistryKey key(rootSubkey_, KEY_QUERY_VALUE);

    DWORD valueCount = 0;
    DWORD maxValueNameLength = 0;
    auto status = RegQueryInfoKeyW(
        key.get(),
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        &valueCount,
        &maxValueNameLength,
        nullptr,
        nullptr,
        nullptr);
    if (status != ERROR_SUCCESS) {
        throwWin32RegistrationError("RegQueryInfoKeyW", status);
    }

    std::vector<ModuleRegistrationRecord> records;
    for (DWORD index = 0; index < valueCount; ++index) {
        std::wstring valueName(maxValueNameLength + 1u, L'\0');
        DWORD valueNameLength = static_cast<DWORD>(valueName.size());
        status = RegEnumValueW(
            key.get(),
            index,
            valueName.data(),
            &valueNameLength,
            nullptr,
            nullptr,
            nullptr,
            nullptr);
        if (status != ERROR_SUCCESS) {
            throwWin32RegistrationError("RegEnumValueW", status);
        }

        valueName.resize(valueNameLength);
        DWORD type = 0;
        DWORD byteCount = 0;
        status = RegQueryValueExW(
            key.get(),
            valueName.c_str(),
            nullptr,
            &type,
            nullptr,
            &byteCount);
        if (status != ERROR_SUCCESS) {
            throwWin32RegistrationError("RegQueryValueExW", status);
        }
        if (type != REG_SZ && type != REG_EXPAND_SZ) {
            throw std::runtime_error("Module registration value is not a string.");
        }

        std::wstring payload(byteCount / sizeof(wchar_t), L'\0');
        status = RegQueryValueExW(
            key.get(),
            valueName.c_str(),
            nullptr,
            &type,
            reinterpret_cast<BYTE*>(payload.data()),
            &byteCount);
        if (status != ERROR_SUCCESS) {
            throwWin32RegistrationError("RegQueryValueExW", status);
        }
        if (!payload.empty() && payload.back() == L'\0') {
            payload.pop_back();
        }

        records.push_back(
            nlohmann::json::parse(wideToUtf8Registration(payload)).get<ModuleRegistrationRecord>());
    }

    return records;
}

void RegistryModuleRegistrationStore::save(const ModuleRegistrationRecord& record)
{
    const auto valueName = utf8ToWideRegistration(record.moduleID);
    const auto payload = utf8ToWideRegistration(nlohmann::json(record).dump());

    std::lock_guard<std::mutex> lock(mutex_);
    RegistrationRegistryKey key(rootSubkey_, KEY_SET_VALUE);
    const auto byteCount = static_cast<DWORD>((payload.size() + 1u) * sizeof(wchar_t));
    const auto status = RegSetValueExW(
        key.get(),
        valueName.c_str(),
        0,
        REG_SZ,
        reinterpret_cast<const BYTE*>(payload.c_str()),
        byteCount);
    if (status != ERROR_SUCCESS) {
        throwWin32RegistrationError("RegSetValueExW", status);
    }
}

void RegistryModuleRegistrationStore::remove(const std::string& moduleID)
{
    const auto valueName = utf8ToWideRegistration(moduleID);

    std::lock_guard<std::mutex> lock(mutex_);
    RegistrationRegistryKey key(rootSubkey_, KEY_SET_VALUE);
    const auto status = RegDeleteValueW(key.get(), valueName.c_str());
    if (status != ERROR_SUCCESS && status != ERROR_FILE_NOT_FOUND) {
        throwWin32RegistrationError("RegDeleteValueW", status);
    }
}

std::string CngSha256ManifestDigestProvider::canonicalManifestJSON(
    const ModuleManifest& manifest) const
{
    return Sha256ManifestDigestProvider{}.canonicalManifestJSON(manifest);
}

std::string CngSha256ManifestDigestProvider::digestManifest(const ModuleManifest& manifest) const
{
    const auto canonical = canonicalManifestJSON(manifest);
    BCryptAlgorithmHandle algorithm;

    DWORD hashLength = 0;
    DWORD resultLength = 0;
    auto status = BCryptGetProperty(
        algorithm.get(),
        BCRYPT_HASH_LENGTH,
        reinterpret_cast<PUCHAR>(&hashLength),
        sizeof(hashLength),
        &resultLength,
        0);
    if (status < 0 || hashLength == 0) {
        throw std::runtime_error("BCryptGetProperty(BCRYPT_HASH_LENGTH) failed.");
    }

    std::vector<std::uint8_t> hash(hashLength);
    status = BCryptHash(
        algorithm.get(),
        nullptr,
        0,
        reinterpret_cast<PUCHAR>(const_cast<char*>(canonical.data())),
        static_cast<ULONG>(canonical.size()),
        hash.data(),
        static_cast<ULONG>(hash.size()));
    if (status < 0) {
        throw std::runtime_error("BCryptHash failed.");
    }

    return bytesToLowerHex(hash);
}

} // namespace Forsetti
