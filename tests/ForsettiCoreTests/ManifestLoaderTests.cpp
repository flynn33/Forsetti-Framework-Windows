// Forsetti Framework for Windows
// Copyright (c) 2026 James Daley. All Rights Reserved.
// Proprietary and Confidential. Patent Pending.

#include "CppUnitTest.h"
#include "ForsettiCore/ManifestLoader.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <stdexcept>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Forsetti;

namespace {
    // Helper: create a temporary directory with optional JSON files
    class TempManifestDir {
        std::filesystem::path dir_;
    public:
        TempManifestDir() {
            dir_ = std::filesystem::temp_directory_path() / "forsetti_test_manifests";
            std::filesystem::create_directories(dir_);
        }

        ~TempManifestDir() {
            std::filesystem::remove_all(dir_);
        }

        std::string path() const { return dir_.string(); }

        void writeFile(const std::string& filename, const std::string& content) {
            std::ofstream f(dir_ / filename);
            f << content;
        }

        void writeManifest(const std::string& filename, const nlohmann::json& j) {
            writeFile(filename, j.dump(2));
        }
    };

    nlohmann::json makeValidManifestJSON(const std::string& moduleID = "com.test.module") {
        return nlohmann::json{
            {"schemaVersion", "1.0"},
            {"moduleID", moduleID},
            {"displayName", "Test Module"},
            {"moduleVersion", {{"major", 1}, {"minor", 0}, {"patch", 0}, {"prerelease", nullptr}}},
            {"moduleType", "service"},
            {"supportedPlatforms", nlohmann::json::array({"Windows"})},
            {"minForsettiVersion", {{"major", 0}, {"minor", 1}, {"patch", 0}, {"prerelease", nullptr}}},
            {"maxForsettiVersion", nullptr},
            {"capabilitiesRequested", nlohmann::json::array({"storage"})},
            {"iapProductID", nullptr},
            {"entryPoint", "TestModule"}
        };
    }
}

TEST_CLASS(ManifestLoaderTests)
{
public:

    TEST_METHOD(LooksLikeManifest_ValidManifest)
    {
        auto j = makeValidManifestJSON();
        Assert::IsTrue(ManifestLoader::looksLikeManifestJSON(j));
    }

    TEST_METHOD(LooksLikeManifest_MissingSchemaVersion)
    {
        nlohmann::json j = {{"moduleID", "test"}, {"displayName", "Test"}};
        Assert::IsFalse(ManifestLoader::looksLikeManifestJSON(j));
    }

    TEST_METHOD(LooksLikeManifest_MissingModuleID)
    {
        nlohmann::json j = {{"schemaVersion", "1.0"}, {"displayName", "Test"}};
        Assert::IsFalse(ManifestLoader::looksLikeManifestJSON(j));
    }

    TEST_METHOD(LooksLikeManifest_MissingDisplayName)
    {
        nlohmann::json j = {{"schemaVersion", "1.0"}, {"moduleID", "test"}};
        Assert::IsFalse(ManifestLoader::looksLikeManifestJSON(j));
    }

    TEST_METHOD(LooksLikeManifest_NotAnObject)
    {
        nlohmann::json j = nlohmann::json::array({1, 2, 3});
        Assert::IsFalse(ManifestLoader::looksLikeManifestJSON(j));
    }

    TEST_METHOD(LoadManifests_DirectoryNotFound)
    {
        Assert::ExpectException<ManifestLoaderException>([]() {
            ManifestLoader::loadManifests("C:/nonexistent_forsetti_test_dir_12345");
        });
    }

    TEST_METHOD(LoadManifests_EmptyDirectory)
    {
        TempManifestDir dir;
        auto manifests = ManifestLoader::loadManifests(dir.path());
        Assert::AreEqual(size_t(0), manifests.size());
    }

    TEST_METHOD(LoadManifests_SingleValidManifest)
    {
        TempManifestDir dir;
        dir.writeManifest("module.json", makeValidManifestJSON("com.test.single"));

        auto manifests = ManifestLoader::loadManifests(dir.path());
        Assert::AreEqual(size_t(1), manifests.size());
        Assert::AreEqual(std::string("com.test.single"), manifests[0].moduleID);
    }

    TEST_METHOD(LoadManifests_MultipleManifests)
    {
        TempManifestDir dir;
        dir.writeManifest("mod1.json", makeValidManifestJSON("com.test.mod1"));
        dir.writeManifest("mod2.json", makeValidManifestJSON("com.test.mod2"));

        auto manifests = ManifestLoader::loadManifests(dir.path());
        Assert::AreEqual(size_t(2), manifests.size());
    }

    TEST_METHOD(LoadManifests_DuplicateModuleID_Throws)
    {
        TempManifestDir dir;
        dir.writeManifest("mod1.json", makeValidManifestJSON("com.test.dupe"));
        dir.writeManifest("mod2.json", makeValidManifestJSON("com.test.dupe"));

        Assert::ExpectException<ManifestLoaderException>([&dir]() {
            ManifestLoader::loadManifests(dir.path());
        });
    }

    TEST_METHOD(LoadManifests_SkipsInvalidJSON)
    {
        TempManifestDir dir;
        dir.writeFile("broken.json", "{ this is not valid json }}}");
        dir.writeManifest("valid.json", makeValidManifestJSON("com.test.valid"));

        auto manifests = ManifestLoader::loadManifests(dir.path());
        Assert::AreEqual(size_t(1), manifests.size());
    }

    TEST_METHOD(LoadManifests_SkipsNonManifestJSON)
    {
        TempManifestDir dir;
        dir.writeFile("config.json", R"({"setting": "value"})");
        dir.writeManifest("module.json", makeValidManifestJSON("com.test.module"));

        auto manifests = ManifestLoader::loadManifests(dir.path());
        Assert::AreEqual(size_t(1), manifests.size());
    }

    TEST_METHOD(LoadManifests_SkipsNonJsonFiles)
    {
        TempManifestDir dir;
        dir.writeFile("readme.txt", "not a json file");
        dir.writeManifest("module.json", makeValidManifestJSON("com.test.module"));

        auto manifests = ManifestLoader::loadManifests(dir.path());
        Assert::AreEqual(size_t(1), manifests.size());
    }

    TEST_METHOD(LoadManifests_ParsesAllRequiredFields)
    {
        TempManifestDir dir;
        auto j = makeValidManifestJSON("com.test.fields");
        j["displayName"] = "Field Test Module";
        j["moduleType"] = "ui";
        j["entryPoint"] = "FieldTestEntry";
        j["iapProductID"] = "com.test.iap";
        dir.writeManifest("fields.json", j);

        auto manifests = ManifestLoader::loadManifests(dir.path());
        Assert::AreEqual(size_t(1), manifests.size());

        const auto& m = manifests[0];
        Assert::AreEqual(std::string("1.0"), m.schemaVersion);
        Assert::AreEqual(std::string("com.test.fields"), m.moduleID);
        Assert::AreEqual(std::string("Field Test Module"), m.displayName);
        Assert::IsTrue(m.moduleType == ModuleType::UI);
        Assert::AreEqual(std::string("FieldTestEntry"), m.entryPoint);
        Assert::IsTrue(m.iapProductID.has_value());
        Assert::AreEqual(std::string("com.test.iap"), m.iapProductID.value());
    }

    TEST_METHOD(PlatformFromString_RejectsApplePlatforms)
    {
        Assert::ExpectException<std::invalid_argument>([]() {
            (void)platformFromString("iOS");
        });

        Assert::ExpectException<std::invalid_argument>([]() {
            (void)platformFromString("macOS");
        });
    }

    TEST_METHOD(LoadManifests_RejectsIOSPlatform)
    {
        TempManifestDir dir;
        auto j = makeValidManifestJSON("com.test.ios");
        j["supportedPlatforms"] = nlohmann::json::array({"iOS"});
        dir.writeManifest("ios.json", j);

        Assert::ExpectException<ManifestLoaderException>([&dir]() {
            (void)ManifestLoader::loadManifests(dir.path());
        });
    }

    TEST_METHOD(LoadManifests_RejectsMacOSPlatform)
    {
        TempManifestDir dir;
        auto j = makeValidManifestJSON("com.test.macos");
        j["supportedPlatforms"] = nlohmann::json::array({"macOS"});
        dir.writeManifest("macos.json", j);

        Assert::ExpectException<ManifestLoaderException>([&dir]() {
            (void)ManifestLoader::loadManifests(dir.path());
        });
    }

    TEST_METHOD(LoadManifests_RejectsMixedNonWindowsPlatform)
    {
        TempManifestDir dir;
        auto j = makeValidManifestJSON("com.test.mixed");
        j["supportedPlatforms"] = nlohmann::json::array({"Windows", "iOS"});
        dir.writeManifest("mixed.json", j);

        Assert::ExpectException<ManifestLoaderException>([&dir]() {
            (void)ManifestLoader::loadManifests(dir.path());
        });
    }

    TEST_METHOD(LoadManifests_RejectsMaxVersionLowerThanMinVersion)
    {
        TempManifestDir dir;
        auto j = makeValidManifestJSON("com.test.version-range");
        j["minForsettiVersion"] = {{"major", 2}, {"minor", 0}, {"patch", 0}, {"prerelease", nullptr}};
        j["maxForsettiVersion"] = {{"major", 1}, {"minor", 0}, {"patch", 0}, {"prerelease", nullptr}};
        dir.writeManifest("range.json", j);

        Assert::ExpectException<ManifestLoaderException>([&dir]() {
            (void)ManifestLoader::loadManifests(dir.path());
        });
    }

    TEST_METHOD(LoadManifests_RejectsMissingRequiredManifestField)
    {
        TempManifestDir dir;
        auto j = makeValidManifestJSON("com.test.missing-entry");
        j.erase("entryPoint");
        dir.writeManifest("missing-entry.json", j);

        Assert::ExpectException<ManifestLoaderException>([&dir]() {
            (void)ManifestLoader::loadManifests(dir.path());
        });
    }

    TEST_METHOD(LoadManifests_RejectsNegativeVersionComponent)
    {
        TempManifestDir dir;
        auto j = makeValidManifestJSON("com.test.negative-version");
        j["moduleVersion"] = {{"major", 1}, {"minor", -1}, {"patch", 0}, {"prerelease", nullptr}};
        dir.writeManifest("negative-version.json", j);

        Assert::ExpectException<ManifestLoaderException>([&dir]() {
            (void)ManifestLoader::loadManifests(dir.path());
        });
    }

    TEST_METHOD(LoadManifests_RejectsBlankModuleID)
    {
        TempManifestDir dir;
        auto j = makeValidManifestJSON();
        j["moduleID"] = "   ";
        dir.writeManifest("blank-id.json", j);

        Assert::ExpectException<ManifestLoaderException>([&dir]() {
            (void)ManifestLoader::loadManifests(dir.path());
        });
    }

    TEST_METHOD(LoadManifests_RejectsUnsafeModuleID)
    {
        TempManifestDir dir;
        auto j = makeValidManifestJSON();
        j["moduleID"] = "../com.test.module";
        dir.writeManifest("unsafe-id.json", j);

        Assert::ExpectException<ManifestLoaderException>([&dir]() {
            (void)ManifestLoader::loadManifests(dir.path());
        });
    }

    TEST_METHOD(LoadManifests_RejectsBlankEntryPoint)
    {
        TempManifestDir dir;
        auto j = makeValidManifestJSON("com.test.blank-entry");
        j["entryPoint"] = "";
        dir.writeManifest("blank-entry.json", j);

        Assert::ExpectException<ManifestLoaderException>([&dir]() {
            (void)ManifestLoader::loadManifests(dir.path());
        });
    }

    TEST_METHOD(LoadManifests_RejectsUnsafeEntryPoint)
    {
        TempManifestDir dir;
        auto j = makeValidManifestJSON("com.test.unsafe-entry");
        j["entryPoint"] = "../TestModule";
        dir.writeManifest("unsafe-entry.json", j);

        Assert::ExpectException<ManifestLoaderException>([&dir]() {
            (void)ManifestLoader::loadManifests(dir.path());
        });
    }
};
