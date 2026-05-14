// Forsetti Framework for Windows
// Copyright (c) 2026 James Daley. All Rights Reserved.
// Proprietary and Confidential. Patent Pending.

#include "ForsettiPlatform/PlatformServices.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <wincrypt.h>
#include <winhttp.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <future>
#include <stdexcept>
#include <utility>

namespace Forsetti {

namespace {

constexpr wchar_t RegistryStorageRoot[] = L"Software\\Forsetti\\Framework\\Storage";
constexpr wchar_t SecureStorageRoot[] = L"Software\\Forsetti\\Framework\\SecureStorage";

std::wstring utf8ToWide(const std::string& value)
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

    std::wstring result(static_cast<size_t>(size), L'\0');
    MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        value.data(),
        static_cast<int>(value.size()),
        result.data(),
        size);
    return result;
}

std::string wideToUtf8(const std::wstring& value)
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

    std::string result(static_cast<size_t>(size), '\0');
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

[[noreturn]] void throwWin32Error(const std::string& operation, DWORD error)
{
    throw std::runtime_error(operation + " failed with Windows error " + std::to_string(error));
}

class RegistryKey final {
public:
    RegistryKey(const std::wstring& subkey, REGSAM access)
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
            throwWin32Error("RegCreateKeyExW", status);
        }
    }

    RegistryKey(const RegistryKey&) = delete;
    RegistryKey& operator=(const RegistryKey&) = delete;

    ~RegistryKey()
    {
        if (key_) {
            RegCloseKey(key_);
        }
    }

    [[nodiscard]] HKEY get() const noexcept
    {
        return key_;
    }

private:
    HKEY key_ = nullptr;
};

std::wstring validateStorageKey(const std::string& key)
{
    if (key.empty()) {
        throw std::invalid_argument("Storage key must not be empty.");
    }
    if (key.size() > 128) {
        throw std::invalid_argument("Storage key is too long.");
    }
    for (const unsigned char ch : key) {
        if (ch < 0x20 || ch == '\\' || ch == '/') {
            throw std::invalid_argument("Storage key contains an unsafe character.");
        }
    }
    return utf8ToWide(key);
}

std::vector<uint8_t> queryRegistryBinaryValue(HKEY key, const std::wstring& valueName)
{
    DWORD type = 0;
    DWORD byteCount = 0;
    auto status = RegQueryValueExW(
        key,
        valueName.c_str(),
        nullptr,
        &type,
        nullptr,
        &byteCount);

    if (status == ERROR_FILE_NOT_FOUND) {
        return {};
    }
    if (status != ERROR_SUCCESS) {
        throwWin32Error("RegQueryValueExW", status);
    }
    if (type != REG_BINARY) {
        throw std::runtime_error("Registry value is not binary data.");
    }

    std::vector<uint8_t> data(byteCount);
    status = RegQueryValueExW(
        key,
        valueName.c_str(),
        nullptr,
        &type,
        data.data(),
        &byteCount);
    if (status != ERROR_SUCCESS) {
        throwWin32Error("RegQueryValueExW", status);
    }
    data.resize(byteCount);
    return data;
}

bool registryValueExists(HKEY key, const std::wstring& valueName)
{
    const auto status = RegQueryValueExW(
        key,
        valueName.c_str(),
        nullptr,
        nullptr,
        nullptr,
        nullptr);
    return status == ERROR_SUCCESS;
}

class LocalAllocBlob final {
public:
    explicit LocalAllocBlob(DATA_BLOB blob) noexcept
        : blob_(blob)
    {
    }

    LocalAllocBlob(const LocalAllocBlob&) = delete;
    LocalAllocBlob& operator=(const LocalAllocBlob&) = delete;

    ~LocalAllocBlob()
    {
        if (blob_.pbData) {
            LocalFree(blob_.pbData);
        }
    }

    [[nodiscard]] const DATA_BLOB& get() const noexcept
    {
        return blob_;
    }

private:
    DATA_BLOB blob_{};
};

bool isReservedDeviceName(std::string name)
{
    const auto dot = name.find('.');
    if (dot != std::string::npos) {
        name = name.substr(0, dot);
    }

    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char ch) {
        return static_cast<char>(std::toupper(ch));
    });

    static constexpr std::array Reserved = {
        "CON", "PRN", "AUX", "NUL",
        "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
        "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"
    };

    return std::find(Reserved.begin(), Reserved.end(), name) != Reserved.end();
}

std::filesystem::path absoluteNormalizedPath(const std::filesystem::path& path)
{
    return std::filesystem::absolute(path).lexically_normal();
}

std::optional<std::wstring> environmentVariable(const wchar_t* name)
{
    const DWORD required = GetEnvironmentVariableW(name, nullptr, 0);
    if (required == 0) {
        return std::nullopt;
    }

    std::wstring value(required, L'\0');
    const DWORD written = GetEnvironmentVariableW(name, value.data(), required);
    if (written == 0 || written >= required) {
        return std::nullopt;
    }

    value.resize(written);
    return value;
}

} // namespace

// ---------------------------------------------------------------------------
// WinHttpNetworkingService
// ---------------------------------------------------------------------------

std::future<std::vector<uint8_t>> WinHttpNetworkingService::data(
    const std::string& url,
    const std::map<std::string, std::string>& headers)
{
    return std::async(std::launch::async, [url, headers]() {
        const auto wideUrl = utf8ToWide(url);

        URL_COMPONENTS components{};
        components.dwStructSize = sizeof(components);
        components.dwSchemeLength = static_cast<DWORD>(-1);
        components.dwHostNameLength = static_cast<DWORD>(-1);
        components.dwUrlPathLength = static_cast<DWORD>(-1);
        components.dwExtraInfoLength = static_cast<DWORD>(-1);

        if (!WinHttpCrackUrl(wideUrl.c_str(), 0, 0, &components)) {
            throwWin32Error("WinHttpCrackUrl", GetLastError());
        }

        if (components.nScheme != INTERNET_SCHEME_HTTP &&
            components.nScheme != INTERNET_SCHEME_HTTPS) {
            throw std::invalid_argument("Only HTTP and HTTPS URLs are supported.");
        }

        const std::wstring host(components.lpszHostName, components.dwHostNameLength);
        std::wstring path(components.lpszUrlPath, components.dwUrlPathLength);
        if (components.dwExtraInfoLength > 0) {
            path.append(components.lpszExtraInfo, components.dwExtraInfoLength);
        }
        if (path.empty()) {
            path = L"/";
        }

        struct InternetHandle final {
            HINTERNET handle = nullptr;
            explicit InternetHandle(HINTERNET h) noexcept : handle(h) {}
            InternetHandle(const InternetHandle&) = delete;
            InternetHandle& operator=(const InternetHandle&) = delete;
            ~InternetHandle() { if (handle) WinHttpCloseHandle(handle); }
            [[nodiscard]] operator HINTERNET() const noexcept { return handle; }
        };

        InternetHandle session(WinHttpOpen(
            L"Forsetti/1.0",
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS,
            0));
        if (!session.handle) {
            throwWin32Error("WinHttpOpen", GetLastError());
        }

        InternetHandle connection(WinHttpConnect(
            session,
            host.c_str(),
            components.nPort,
            0));
        if (!connection.handle) {
            throwWin32Error("WinHttpConnect", GetLastError());
        }

        const DWORD requestFlags =
            components.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0;
        InternetHandle request(WinHttpOpenRequest(
            connection,
            L"GET",
            path.c_str(),
            nullptr,
            WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            requestFlags));
        if (!request.handle) {
            throwWin32Error("WinHttpOpenRequest", GetLastError());
        }

        std::wstring headerBlock;
        for (const auto& [name, value] : headers) {
            if (name.find('\r') != std::string::npos || name.find('\n') != std::string::npos ||
                value.find('\r') != std::string::npos || value.find('\n') != std::string::npos) {
                throw std::invalid_argument("HTTP headers must not contain line breaks.");
            }
            headerBlock += utf8ToWide(name);
            headerBlock += L": ";
            headerBlock += utf8ToWide(value);
            headerBlock += L"\r\n";
        }

        const wchar_t* headerPointer =
            headerBlock.empty() ? WINHTTP_NO_ADDITIONAL_HEADERS : headerBlock.c_str();
        const DWORD headerLength =
            headerBlock.empty() ? 0 : static_cast<DWORD>(headerBlock.size());

        if (!WinHttpSendRequest(
                request,
                headerPointer,
                headerLength,
                WINHTTP_NO_REQUEST_DATA,
                0,
                0,
                0)) {
            throwWin32Error("WinHttpSendRequest", GetLastError());
        }

        if (!WinHttpReceiveResponse(request, nullptr)) {
            throwWin32Error("WinHttpReceiveResponse", GetLastError());
        }

        DWORD statusCode = 0;
        DWORD statusCodeSize = sizeof(statusCode);
        if (!WinHttpQueryHeaders(
                request,
                WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                WINHTTP_HEADER_NAME_BY_INDEX,
                &statusCode,
                &statusCodeSize,
                WINHTTP_NO_HEADER_INDEX)) {
            throwWin32Error("WinHttpQueryHeaders", GetLastError());
        }

        if (statusCode < 200 || statusCode >= 300) {
            throw std::runtime_error("HTTP request failed with status " + std::to_string(statusCode));
        }

        std::vector<uint8_t> response;
        DWORD available = 0;
        do {
            if (!WinHttpQueryDataAvailable(request, &available)) {
                throwWin32Error("WinHttpQueryDataAvailable", GetLastError());
            }
            if (available == 0) {
                break;
            }

            const auto offset = response.size();
            response.resize(offset + available);
            DWORD bytesRead = 0;
            if (!WinHttpReadData(
                    request,
                    response.data() + offset,
                    available,
                    &bytesRead)) {
                throwWin32Error("WinHttpReadData", GetLastError());
            }
            response.resize(offset + bytesRead);
        } while (available > 0);

        return response;
    });
}

// ---------------------------------------------------------------------------
// RegistryStorageService
// ---------------------------------------------------------------------------

RegistryStorageService::RegistryStorageService()
    : rootSubkey_(RegistryStorageRoot)
{
}

RegistryStorageService::RegistryStorageService(std::wstring rootSubkey)
    : rootSubkey_(std::move(rootSubkey))
{
}

void RegistryStorageService::set(const std::string& key, const std::string& value)
{
    const auto valueName = validateStorageKey(key);
    const auto valueData = utf8ToWide(value);

    std::lock_guard<std::mutex> lock(mutex_);
    RegistryKey registryKey(rootSubkey_, KEY_SET_VALUE);
    const auto byteCount = static_cast<DWORD>((valueData.size() + 1) * sizeof(wchar_t));
    const auto status = RegSetValueExW(
        registryKey.get(),
        valueName.c_str(),
        0,
        REG_SZ,
        reinterpret_cast<const BYTE*>(valueData.c_str()),
        byteCount);
    if (status != ERROR_SUCCESS) {
        throwWin32Error("RegSetValueExW", status);
    }
}

std::optional<std::string> RegistryStorageService::get(const std::string& key)
{
    const auto valueName = validateStorageKey(key);

    std::lock_guard<std::mutex> lock(mutex_);
    RegistryKey registryKey(rootSubkey_, KEY_QUERY_VALUE);

    DWORD type = 0;
    DWORD byteCount = 0;
    auto status = RegQueryValueExW(
        registryKey.get(),
        valueName.c_str(),
        nullptr,
        &type,
        nullptr,
        &byteCount);

    if (status == ERROR_FILE_NOT_FOUND) {
        return std::nullopt;
    }
    if (status != ERROR_SUCCESS) {
        throwWin32Error("RegQueryValueExW", status);
    }
    if (type != REG_SZ && type != REG_EXPAND_SZ) {
        throw std::runtime_error("Registry value is not a string.");
    }

    std::wstring buffer(byteCount / sizeof(wchar_t), L'\0');
    status = RegQueryValueExW(
        registryKey.get(),
        valueName.c_str(),
        nullptr,
        &type,
        reinterpret_cast<BYTE*>(buffer.data()),
        &byteCount);
    if (status != ERROR_SUCCESS) {
        throwWin32Error("RegQueryValueExW", status);
    }

    if (!buffer.empty() && buffer.back() == L'\0') {
        buffer.pop_back();
    }
    return wideToUtf8(buffer);
}

void RegistryStorageService::remove(const std::string& key)
{
    const auto valueName = validateStorageKey(key);

    std::lock_guard<std::mutex> lock(mutex_);
    RegistryKey registryKey(rootSubkey_, KEY_SET_VALUE);
    const auto status = RegDeleteValueW(registryKey.get(), valueName.c_str());
    if (status != ERROR_SUCCESS && status != ERROR_FILE_NOT_FOUND) {
        throwWin32Error("RegDeleteValueW", status);
    }
}

// ---------------------------------------------------------------------------
// DpapiSecureStorageService
// ---------------------------------------------------------------------------

DpapiSecureStorageService::DpapiSecureStorageService()
    : rootSubkey_(SecureStorageRoot)
{
}

DpapiSecureStorageService::DpapiSecureStorageService(std::wstring rootSubkey)
    : rootSubkey_(std::move(rootSubkey))
{
}

void DpapiSecureStorageService::set(const std::string& key, const std::vector<uint8_t>& data)
{
    const auto valueName = validateStorageKey(key);
    std::vector<uint8_t> payload;
    payload.reserve(data.size() + 1);
    payload.push_back(0x01);
    payload.insert(payload.end(), data.begin(), data.end());

    DATA_BLOB plain{};
    plain.cbData = static_cast<DWORD>(payload.size());
    plain.pbData = payload.data();

    DATA_BLOB protectedBlob{};
    if (!CryptProtectData(
            &plain,
            L"Forsetti secure storage value",
            nullptr,
            nullptr,
            nullptr,
            CRYPTPROTECT_UI_FORBIDDEN,
            &protectedBlob)) {
        throwWin32Error("CryptProtectData", GetLastError());
    }

    LocalAllocBlob encrypted(protectedBlob);

    std::lock_guard<std::mutex> lock(mutex_);
    RegistryKey registryKey(rootSubkey_, KEY_SET_VALUE);
    const auto& blob = encrypted.get();
    const auto status = RegSetValueExW(
        registryKey.get(),
        valueName.c_str(),
        0,
        REG_BINARY,
        blob.pbData,
        blob.cbData);
    if (status != ERROR_SUCCESS) {
        throwWin32Error("RegSetValueExW", status);
    }
}

std::optional<std::vector<uint8_t>> DpapiSecureStorageService::get(const std::string& key)
{
    const auto valueName = validateStorageKey(key);

    std::lock_guard<std::mutex> lock(mutex_);
    RegistryKey registryKey(rootSubkey_, KEY_QUERY_VALUE);
    if (!registryValueExists(registryKey.get(), valueName)) {
        return std::nullopt;
    }

    auto encrypted = queryRegistryBinaryValue(registryKey.get(), valueName);
    DATA_BLOB encryptedBlob{};
    encryptedBlob.cbData = static_cast<DWORD>(encrypted.size());
    encryptedBlob.pbData = encrypted.empty() ? nullptr : encrypted.data();

    DATA_BLOB plain{};
    if (!CryptUnprotectData(
            &encryptedBlob,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            CRYPTPROTECT_UI_FORBIDDEN,
            &plain)) {
        throwWin32Error("CryptUnprotectData", GetLastError());
    }

    LocalAllocBlob decrypted(plain);
    const auto& blob = decrypted.get();
    if (blob.cbData == 0 || !blob.pbData || blob.pbData[0] != 0x01) {
        throw std::runtime_error("Secure storage value is not in the expected format.");
    }
    if (blob.cbData == 1) {
        return std::vector<uint8_t>{};
    }
    return std::vector<uint8_t>(blob.pbData + 1, blob.pbData + blob.cbData);
}

void DpapiSecureStorageService::remove(const std::string& key)
{
    const auto valueName = validateStorageKey(key);

    std::lock_guard<std::mutex> lock(mutex_);
    RegistryKey registryKey(rootSubkey_, KEY_SET_VALUE);
    const auto status = RegDeleteValueW(registryKey.get(), valueName.c_str());
    if (status != ERROR_SUCCESS && status != ERROR_FILE_NOT_FOUND) {
        throwWin32Error("RegDeleteValueW", status);
    }
}

// ---------------------------------------------------------------------------
// LocalFileExportService
// ---------------------------------------------------------------------------

LocalFileExportService::LocalFileExportService()
    : exportDirectory_(defaultExportDirectory())
{
}

LocalFileExportService::LocalFileExportService(std::filesystem::path exportDirectory)
    : exportDirectory_(absoluteNormalizedPath(exportDirectory))
{
}

bool LocalFileExportService::exportData(
    const std::vector<uint8_t>& data,
    const std::string& filename)
{
    try {
        std::filesystem::create_directories(exportDirectory_);
        const auto root = absoluteNormalizedPath(exportDirectory_);
        const auto safeName = sanitizedFilename(filename);
        const auto target = absoluteNormalizedPath(root / safeName);

        if (target.parent_path() != root) {
            return false;
        }

        std::ofstream stream(target, std::ios::binary | std::ios::trunc);
        if (!stream) {
            return false;
        }
        stream.write(
            reinterpret_cast<const char*>(data.data()),
            static_cast<std::streamsize>(data.size()));
        return stream.good();
    } catch (...) {
        return false;
    }
}

const std::filesystem::path& LocalFileExportService::exportDirectory() const noexcept
{
    return exportDirectory_;
}

std::filesystem::path LocalFileExportService::defaultExportDirectory()
{
    const auto localAppData = environmentVariable(L"LOCALAPPDATA");
    if (localAppData.has_value() && !localAppData->empty()) {
        return absoluteNormalizedPath(
            std::filesystem::path(localAppData.value()) / "Forsetti" / "Exports");
    }
    return absoluteNormalizedPath(
        std::filesystem::temp_directory_path() / "Forsetti" / "Exports");
}

std::string LocalFileExportService::sanitizedFilename(const std::string& filename)
{
    auto name = std::filesystem::path(filename).filename().string();
    if (name.empty() || name == "." || name == "..") {
        name = "export.bin";
    }

    std::string sanitized;
    sanitized.reserve(name.size());
    for (const unsigned char ch : name) {
        if (std::isalnum(ch) || ch == '.' || ch == '-' || ch == '_') {
            sanitized.push_back(static_cast<char>(ch));
        } else {
            sanitized.push_back('_');
        }
    }

    while (!sanitized.empty() &&
           (sanitized.back() == '.' || sanitized.back() == ' ')) {
        sanitized.pop_back();
    }

    if (sanitized.empty() || sanitized == "." || sanitized == "..") {
        sanitized = "export.bin";
    }

    if (sanitized.size() > 128) {
        sanitized.resize(128);
        while (!sanitized.empty() &&
               (sanitized.back() == '.' || sanitized.back() == ' ')) {
            sanitized.pop_back();
        }
    }

    if (isReservedDeviceName(sanitized)) {
        sanitized += "_";
    }

    return sanitized;
}

// ---------------------------------------------------------------------------
// NoopTelemetryService
// ---------------------------------------------------------------------------

void NoopTelemetryService::trackEvent(
    const std::string& /*name*/,
    const std::map<std::string, std::string>& /*properties*/)
{
}

} // namespace Forsetti
