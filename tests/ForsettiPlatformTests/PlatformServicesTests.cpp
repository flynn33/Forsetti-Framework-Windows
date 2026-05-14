// Forsetti Framework for Windows
// Copyright (c) 2026 James Daley. All Rights Reserved.
// Proprietary and Confidential. Patent Pending.

#include "CppUnitTest.h"
#include "ForsettiPlatform/PlatformServices.h"
#include "ForsettiPlatform/DefaultPlatformServices.h"
#include "ForsettiCore/ForsettiServiceContainer.h"

#include <chrono>
#include <filesystem>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Forsetti;

namespace {

    std::wstring makeRegistryTestRoot(const std::wstring& suffix)
    {
        const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
        return L"Software\\Forsetti\\Tests\\PlatformServices\\" +
            std::to_wstring(now) + L"\\" + suffix;
    }

    class TempExportDir {
        std::filesystem::path dir_;
    public:
        TempExportDir()
        {
            const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
            dir_ = std::filesystem::temp_directory_path() /
                ("forsetti-platform-export-" + std::to_string(now));
            std::filesystem::create_directories(dir_);
        }

        ~TempExportDir()
        {
            std::error_code ignored;
            std::filesystem::remove_all(dir_, ignored);
        }

        [[nodiscard]] const std::filesystem::path& path() const noexcept
        {
            return dir_;
        }
    };

    size_t fileCount(const std::filesystem::path& dir)
    {
        size_t count = 0;
        for (const auto& entry : std::filesystem::directory_iterator(dir)) {
            if (entry.is_regular_file()) {
                ++count;
            }
        }
        return count;
    }
}

TEST_CLASS(RegistryStorageServiceTests)
{
public:

    TEST_METHOD(SetAndGet)
    {
        RegistryStorageService service(makeRegistryTestRoot(L"SetAndGet"));
        service.set("key1", "value1");
        auto result = service.get("key1");
        Assert::IsTrue(result.has_value());
        Assert::AreEqual(std::string("value1"), result.value());
    }

    TEST_METHOD(GetNonexistent_ReturnsNullopt)
    {
        RegistryStorageService service(makeRegistryTestRoot(L"GetNonexistent"));
        auto result = service.get("nonexistent");
        Assert::IsFalse(result.has_value());
    }

    TEST_METHOD(SetOverwrites)
    {
        RegistryStorageService service(makeRegistryTestRoot(L"SetOverwrites"));
        service.set("key", "first");
        service.set("key", "second");
        Assert::AreEqual(std::string("second"), service.get("key").value());
    }

    TEST_METHOD(Remove)
    {
        RegistryStorageService service(makeRegistryTestRoot(L"Remove"));
        service.set("key", "value");
        service.remove("key");
        Assert::IsFalse(service.get("key").has_value());
    }

    TEST_METHOD(RemoveNonexistent_NoError)
    {
        RegistryStorageService service(makeRegistryTestRoot(L"RemoveNonexistent"));
        service.remove("nonexistent");
        Assert::IsTrue(true);
    }

    TEST_METHOD(MultipleKeys)
    {
        RegistryStorageService service(makeRegistryTestRoot(L"MultipleKeys"));
        service.set("a", "1");
        service.set("b", "2");
        service.set("c", "3");

        Assert::AreEqual(std::string("1"), service.get("a").value());
        Assert::AreEqual(std::string("2"), service.get("b").value());
        Assert::AreEqual(std::string("3"), service.get("c").value());
    }

    TEST_METHOD(PersistsAcrossInstances)
    {
        const auto root = makeRegistryTestRoot(L"PersistsAcrossInstances");
        RegistryStorageService writer(root);
        writer.set("key", "persisted");

        RegistryStorageService reader(root);
        Assert::AreEqual(std::string("persisted"), reader.get("key").value());
    }

    TEST_METHOD(RejectsUnsafeKeys)
    {
        RegistryStorageService service(makeRegistryTestRoot(L"RejectsUnsafeKeys"));

        Assert::ExpectException<std::invalid_argument>([&service]() {
            service.set("", "value");
        });
        Assert::ExpectException<std::invalid_argument>([&service]() {
            service.set("nested\\key", "value");
        });
        Assert::ExpectException<std::invalid_argument>([&service]() {
            service.get("nested/key");
        });
    }
};

TEST_CLASS(DpapiSecureStorageServiceTests)
{
public:

    TEST_METHOD(SetAndGet)
    {
        DpapiSecureStorageService service(makeRegistryTestRoot(L"SecureSetAndGet"));
        std::vector<uint8_t> data = {0x01, 0x02, 0x03};
        service.set("secret", data);

        auto result = service.get("secret");
        Assert::IsTrue(result.has_value());
        Assert::AreEqual(size_t(3), result.value().size());
        Assert::AreEqual(uint8_t(0x01), result.value()[0]);
        Assert::AreEqual(uint8_t(0x02), result.value()[1]);
        Assert::AreEqual(uint8_t(0x03), result.value()[2]);
    }

    TEST_METHOD(GetNonexistent_ReturnsNullopt)
    {
        DpapiSecureStorageService service(makeRegistryTestRoot(L"SecureGetNonexistent"));
        Assert::IsFalse(service.get("nonexistent").has_value());
    }

    TEST_METHOD(Remove)
    {
        DpapiSecureStorageService service(makeRegistryTestRoot(L"SecureRemove"));
        service.set("key", {0xFF});
        service.remove("key");
        Assert::IsFalse(service.get("key").has_value());
    }

    TEST_METHOD(EmptyData)
    {
        DpapiSecureStorageService service(makeRegistryTestRoot(L"SecureEmptyData"));
        service.set("empty", {});
        auto result = service.get("empty");
        Assert::IsTrue(result.has_value());
        Assert::AreEqual(size_t(0), result.value().size());
    }

    TEST_METHOD(PersistsAcrossInstances)
    {
        const auto root = makeRegistryTestRoot(L"SecurePersistsAcrossInstances");
        DpapiSecureStorageService writer(root);
        writer.set("secret", {0x10, 0x20, 0x30});

        DpapiSecureStorageService reader(root);
        auto result = reader.get("secret");
        Assert::IsTrue(result.has_value());
        Assert::AreEqual(size_t(3), result.value().size());
        Assert::AreEqual(uint8_t(0x20), result.value()[1]);
    }

    TEST_METHOD(RejectsUnsafeKeys)
    {
        DpapiSecureStorageService service(makeRegistryTestRoot(L"SecureRejectsUnsafeKeys"));

        Assert::ExpectException<std::invalid_argument>([&service]() {
            service.set("nested\\key", {0x01});
        });
        Assert::ExpectException<std::invalid_argument>([&service]() {
            service.get("");
        });
    }
};

TEST_CLASS(WinHttpNetworkingServiceTests)
{
public:

    TEST_METHOD(Data_RejectsUnsupportedScheme)
    {
        WinHttpNetworkingService service;
        auto future = service.data("ftp://example.com/file.txt");

        Assert::ExpectException<std::invalid_argument>([&future]() {
            future.get();
        });
    }

    TEST_METHOD(Data_RejectsHeaderLineBreaks)
    {
        WinHttpNetworkingService service;
        auto future = service.data("https://example.com", {{"X-Test", "bad\r\nvalue"}});

        Assert::ExpectException<std::invalid_argument>([&future]() {
            future.get();
        });
    }
};

TEST_CLASS(LocalFileExportServiceTests)
{
public:

    TEST_METHOD(ExportData_WritesFileInsideConfiguredDirectory)
    {
        TempExportDir dir;
        LocalFileExportService service(dir.path());

        const auto result = service.exportData({0x01, 0x02}, "test.bin");

        Assert::IsTrue(result);
        const auto target = dir.path() / "test.bin";
        Assert::IsTrue(std::filesystem::exists(target));
        Assert::AreEqual(uintmax_t(2), std::filesystem::file_size(target));
    }

    TEST_METHOD(ExportData_SanitizesTraversalFilename)
    {
        TempExportDir dir;
        LocalFileExportService service(dir.path());
        const auto outside = dir.path().parent_path() / "escape.bin";
        std::filesystem::remove(outside);

        const auto result = service.exportData({0x01}, "..\\escape.bin");

        Assert::IsTrue(result);
        Assert::IsFalse(std::filesystem::exists(outside));
        Assert::AreEqual(size_t(1), fileCount(dir.path()));
    }

    TEST_METHOD(SanitizesReservedDeviceFilename)
    {
        Assert::AreEqual(
            std::string("CON_"),
            LocalFileExportService::sanitizedFilename("CON"));
    }

    TEST_METHOD(SanitizesEmptyFilename)
    {
        Assert::AreEqual(
            std::string("export.bin"),
            LocalFileExportService::sanitizedFilename(".."));
    }
};

TEST_CLASS(NoopTelemetryServiceTests)
{
public:

    TEST_METHOD(TrackEvent_DoesNotThrow)
    {
        NoopTelemetryService service;
        service.trackEvent("test_event", {{"prop", "value"}});
        Assert::IsTrue(true);
    }
};

TEST_CLASS(DefaultPlatformServicesTests)
{
public:

    TEST_METHOD(RegisterAll_ServicesResolvable)
    {
        ServiceContainer container;
        DefaultForsettiPlatformServices::registerAll(container);

        Assert::IsNotNull(container.resolve<INetworkingService>().get());
        Assert::IsNotNull(container.resolve<IStorageService>().get());
        Assert::IsNotNull(container.resolve<ISecureStorageService>().get());
        Assert::IsNotNull(container.resolve<IFileExportService>().get());
        Assert::IsNotNull(container.resolve<ITelemetryService>().get());
    }
};
