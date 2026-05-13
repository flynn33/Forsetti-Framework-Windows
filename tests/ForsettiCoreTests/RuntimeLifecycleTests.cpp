// Forsetti Framework for Windows
// Copyright (c) 2026 James Daley. All Rights Reserved.
// Proprietary and Confidential. Patent Pending.

#include "CppUnitTest.h"
#include "TestHelpers.h"
#include "ForsettiCore/ForsettiRuntime.h"
#include "ForsettiCore/ForsettiVersion.h"
#include "ForsettiCore/ManifestLoader.h"
#include "ForsettiCore/UISurfaceManager.h"
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Forsetti;
using namespace Forsetti::Tests;

namespace {
    // Creates a temporary manifest directory with example module manifests
    class TempRuntimeDir {
        std::filesystem::path dir_;
    public:
        TempRuntimeDir() {
            dir_ = std::filesystem::temp_directory_path() / "forsetti_runtime_test";
            std::filesystem::create_directories(dir_);
        }
        ~TempRuntimeDir() { std::filesystem::remove_all(dir_); }
        std::string path() const { return dir_.string(); }

        void writeManifest(const std::string& filename, const nlohmann::json& j) {
            std::ofstream f(dir_ / filename);
            f << j.dump(2);
        }
    };

    nlohmann::json makeServiceManifestJSON() {
        return nlohmann::json{
            {"schemaVersion", "1.0"},
            {"moduleID", "com.test.service"},
            {"displayName", "Test Service"},
            {"moduleVersion", {{"major", 0}, {"minor", 1}, {"patch", 0}, {"prerelease", nullptr}}},
            {"moduleType", "service"},
            {"supportedPlatforms", nlohmann::json::array({"Windows"})},
            {"minForsettiVersion", {{"major", 0}, {"minor", 1}, {"patch", 0}, {"prerelease", nullptr}}},
            {"maxForsettiVersion", nullptr},
            {"capabilitiesRequested", nlohmann::json::array()},
            {"iapProductID", nullptr},
            {"entryPoint", "TestServiceModule"}
        };
    }

    nlohmann::json makeManifestJSON(
        const std::string& moduleID,
        const std::string& displayName,
        ModuleType moduleType,
        const std::string& entryPoint) {
        return nlohmann::json{
            {"schemaVersion", "1.0"},
            {"moduleID", moduleID},
            {"displayName", displayName},
            {"moduleVersion", {{"major", 0}, {"minor", 1}, {"patch", 0}, {"prerelease", nullptr}}},
            {"moduleType", Forsetti::to_string(moduleType)},
            {"supportedPlatforms", nlohmann::json::array({"Windows"})},
            {"minForsettiVersion", {{"major", 0}, {"minor", 1}, {"patch", 0}, {"prerelease", nullptr}}},
            {"maxForsettiVersion", nullptr},
            {"capabilitiesRequested", nlohmann::json::array()},
            {"iapProductID", nullptr},
            {"entryPoint", entryPoint}
        };
    }

    ModuleDescriptor makeDescriptor(
        const std::string& moduleID,
        const std::string& displayName,
        ModuleType moduleType,
        SemVer version = SemVer{0, 1, 0}) {
        return ModuleDescriptor{
            .moduleID = moduleID,
            .displayName = displayName,
            .version = version,
            .type = moduleType
        };
    }

    ModuleManifest makeManifest(
        const std::string& moduleID,
        const std::string& displayName,
        ModuleType moduleType,
        const std::string& entryPoint,
        SemVer version = SemVer{0, 1, 0}) {
        return ModuleManifest{
            .schemaVersion = "1.0",
            .moduleID = moduleID,
            .displayName = displayName,
            .moduleVersion = version,
            .moduleType = moduleType,
            .supportedPlatforms = {Platform::Windows},
            .minForsettiVersion = SemVer{0, 1, 0},
            .maxForsettiVersion = std::nullopt,
            .capabilitiesRequested = {},
            .iapProductID = std::nullopt,
            .entryPoint = entryPoint
        };
    }

    void expectModuleManagerError(
        const std::function<void()>& action,
        ModuleManagerError expectedError) {
        try {
            action();
            Assert::Fail(L"Expected ModuleManagerException.");
        } catch (const ModuleManagerException& ex) {
            Assert::IsTrue(ex.error() == expectedError);
        }
    }

    struct RuntimeTestFixture {
        std::shared_ptr<MockEntitlementProvider> entitlements;
        std::shared_ptr<InMemoryEventBus> eventBus;
        std::shared_ptr<InMemoryActivationStore> store;
        std::shared_ptr<UISurfaceManager> surfaceManager;
        std::shared_ptr<ForsettiContext> context;
        std::shared_ptr<CompatibilityChecker> checker;
        std::shared_ptr<RecordingLogger> logger;

        RuntimeTestFixture() {
            entitlements = std::make_shared<MockEntitlementProvider>();
            entitlements->setUnlocked({"com.test.service"});
            eventBus = std::make_shared<InMemoryEventBus>();
            store = std::make_shared<InMemoryActivationStore>();
            surfaceManager = std::make_shared<UISurfaceManager>();

            auto services = std::make_shared<ServiceContainer>();
            logger = std::make_shared<RecordingLogger>();
            auto router = std::make_shared<NoopOverlayRouter>();
            auto guard = std::make_shared<DefaultModuleCommunicationGuard>();
            context = std::make_shared<ForsettiContext>(services, eventBus, logger, router, guard);

            auto policy = std::make_shared<AllowAllCapabilityPolicy>();
            checker = std::make_shared<CompatibilityChecker>(ForsettiVersion::current, policy);
        }

        std::unique_ptr<ModuleManager> makeModuleManager(ModuleRegistry registry) {
            return std::make_unique<ModuleManager>(
                std::move(registry), checker, entitlements, store, surfaceManager, context);
        }
    };
}

TEST_CLASS(RuntimeLifecycleTests)
{
public:

    TEST_METHOD(Runtime_NotBootedByDefault)
    {
        RuntimeTestFixture fix;
        TempRuntimeDir dir;

        ModuleRegistry registry;
        auto runtime = ForsettiRuntime(
            fix.makeModuleManager(std::move(registry)),
            fix.entitlements, fix.eventBus, dir.path());

        Assert::IsFalse(runtime.isBooted());
    }

    TEST_METHOD(Runtime_BootSetsBootedFlag)
    {
        RuntimeTestFixture fix;
        TempRuntimeDir dir;

        ModuleRegistry registry;
        auto runtime = ForsettiRuntime(
            fix.makeModuleManager(std::move(registry)),
            fix.entitlements, fix.eventBus, dir.path());

        runtime.boot();
        Assert::IsTrue(runtime.isBooted());
    }

    TEST_METHOD(Runtime_ShutdownClearsBootedFlag)
    {
        RuntimeTestFixture fix;
        TempRuntimeDir dir;

        ModuleRegistry registry;
        auto runtime = ForsettiRuntime(
            fix.makeModuleManager(std::move(registry)),
            fix.entitlements, fix.eventBus, dir.path());

        runtime.boot();
        Assert::IsTrue(runtime.isBooted());

        runtime.shutdown();
        Assert::IsFalse(runtime.isBooted());
    }

    TEST_METHOD(Runtime_BootDiscoversManifests)
    {
        RuntimeTestFixture fix;
        TempRuntimeDir dir;
        dir.writeManifest("service.json", makeServiceManifestJSON());

        ModuleRegistry registry;
        registry.registerModule("TestServiceModule", []() -> std::unique_ptr<IForsettiModule> {
            auto desc = ModuleDescriptor{
                .moduleID = "com.test.service", .displayName = "Test Service",
                .version = SemVer{0,1,0}, .type = ModuleType::Service};
            auto manifest = ModuleManifest{
                .schemaVersion = "1.0", .moduleID = "com.test.service",
                .displayName = "Test Service", .moduleVersion = SemVer{0,1,0},
                .moduleType = ModuleType::Service,
                .supportedPlatforms = {Platform::Windows},
                .minForsettiVersion = SemVer{0,1,0},
                .capabilitiesRequested = {},
                .entryPoint = "TestServiceModule"};
            return std::make_unique<StubForsettiModule>(desc, manifest);
        });

        auto runtime = ForsettiRuntime(
            fix.makeModuleManager(std::move(registry)),
            fix.entitlements, fix.eventBus, dir.path());

        runtime.boot();

        const auto& manifests = runtime.moduleManager().manifestsByID();
        Assert::AreEqual(size_t(1), manifests.size());
        Assert::IsTrue(manifests.count("com.test.service") > 0);
    }

    TEST_METHOD(Runtime_ActivateAndDeactivateModule)
    {
        RuntimeTestFixture fix;
        TempRuntimeDir dir;
        dir.writeManifest("service.json", makeServiceManifestJSON());

        ModuleRegistry registry;
        registry.registerModule("TestServiceModule", []() -> std::unique_ptr<IForsettiModule> {
            auto desc = ModuleDescriptor{
                .moduleID = "com.test.service", .displayName = "Test Service",
                .version = SemVer{0,1,0}, .type = ModuleType::Service};
            auto manifest = ModuleManifest{
                .schemaVersion = "1.0", .moduleID = "com.test.service",
                .displayName = "Test Service", .moduleVersion = SemVer{0,1,0},
                .moduleType = ModuleType::Service,
                .supportedPlatforms = {Platform::Windows},
                .minForsettiVersion = SemVer{0,1,0},
                .capabilitiesRequested = {},
                .entryPoint = "TestServiceModule"};
            return std::make_unique<StubForsettiModule>(desc, manifest);
        });

        auto runtime = ForsettiRuntime(
            fix.makeModuleManager(std::move(registry)),
            fix.entitlements, fix.eventBus, dir.path());

        runtime.boot();
        runtime.activateModule("com.test.service");
        Assert::IsTrue(runtime.moduleManager().isModuleActive("com.test.service"));

        runtime.deactivateModule("com.test.service");
        Assert::IsFalse(runtime.moduleManager().isModuleActive("com.test.service"));
    }

    TEST_METHOD(Runtime_Reconciliation_DeactivatesRevokedModules)
    {
        RuntimeTestFixture fix;
        TempRuntimeDir dir;
        dir.writeManifest("service.json", makeServiceManifestJSON());

        ModuleRegistry registry;
        registry.registerModule("TestServiceModule", []() -> std::unique_ptr<IForsettiModule> {
            auto desc = ModuleDescriptor{
                .moduleID = "com.test.service", .displayName = "Test Service",
                .version = SemVer{0,1,0}, .type = ModuleType::Service};
            auto manifest = ModuleManifest{
                .schemaVersion = "1.0", .moduleID = "com.test.service",
                .displayName = "Test Service", .moduleVersion = SemVer{0,1,0},
                .moduleType = ModuleType::Service,
                .supportedPlatforms = {Platform::Windows},
                .minForsettiVersion = SemVer{0,1,0},
                .capabilitiesRequested = {},
                .entryPoint = "TestServiceModule"};
            return std::make_unique<StubForsettiModule>(desc, manifest);
        });

        auto runtime = ForsettiRuntime(
            fix.makeModuleManager(std::move(registry)),
            fix.entitlements, fix.eventBus, dir.path());

        runtime.boot();
        runtime.activateModule("com.test.service");
        Assert::IsTrue(runtime.moduleManager().isModuleActive("com.test.service"));

        // Revoke entitlement
        fix.entitlements->setUnlocked({});
        runtime.reconcileActiveModulesWithEntitlements();
        Assert::IsFalse(runtime.moduleManager().isModuleActive("com.test.service"));
    }

    TEST_METHOD(Runtime_UIManifestReturningServiceOnlyFailsActivationWithoutPersistingState)
    {
        RuntimeTestFixture fix;
        fix.entitlements->setUnlocked({"com.test.ui"});

        TempRuntimeDir dir;
        dir.writeManifest("ui.json", makeManifestJSON(
            "com.test.ui", "Test UI", ModuleType::UI, "TestUIModule"));

        ModuleRegistry registry;
        registry.registerModule("TestUIModule", []() -> std::unique_ptr<IForsettiModule> {
            auto desc = makeDescriptor("com.test.ui", "Test UI", ModuleType::UI);
            auto manifest = makeManifest("com.test.ui", "Test UI", ModuleType::UI, "TestUIModule");
            return std::make_unique<StubForsettiModule>(desc, manifest);
        });

        auto runtime = ForsettiRuntime(
            fix.makeModuleManager(std::move(registry)),
            fix.entitlements, fix.eventBus, dir.path());

        runtime.boot();

        expectModuleManagerError([&runtime]() {
            runtime.activateModule("com.test.ui");
        }, ModuleManagerError::ModuleTypeMismatch);

        const auto state = fix.store->loadState();
        Assert::IsFalse(runtime.moduleManager().isModuleActive("com.test.ui"));
        Assert::IsTrue(state.enabledServiceModuleIDs.empty());
        Assert::IsTrue(state.enabledUIModuleIDs.empty());
        Assert::IsFalse(state.selectedUIModuleID.has_value());
    }

    TEST_METHOD(Runtime_AppManifestWithoutAppUIContractFailsActivationWithoutPersistingState)
    {
        RuntimeTestFixture fix;
        fix.entitlements->setUnlocked({"com.test.app"});

        TempRuntimeDir dir;
        dir.writeManifest("app.json", makeManifestJSON(
            "com.test.app", "Test App", ModuleType::App, "TestAppModule"));

        ModuleRegistry registry;
        registry.registerModule("TestAppModule", []() -> std::unique_ptr<IForsettiModule> {
            auto desc = makeDescriptor("com.test.app", "Test App", ModuleType::App);
            auto manifest = makeManifest("com.test.app", "Test App", ModuleType::App, "TestAppModule");
            return std::make_unique<StubForsettiModule>(desc, manifest);
        });

        auto runtime = ForsettiRuntime(
            fix.makeModuleManager(std::move(registry)),
            fix.entitlements, fix.eventBus, dir.path());

        runtime.boot();

        expectModuleManagerError([&runtime]() {
            runtime.activateModule("com.test.app");
        }, ModuleManagerError::ModuleTypeMismatch);

        const auto state = fix.store->loadState();
        Assert::IsFalse(runtime.moduleManager().isModuleActive("com.test.app"));
        Assert::IsTrue(state.enabledServiceModuleIDs.empty());
        Assert::IsTrue(state.enabledUIModuleIDs.empty());
        Assert::IsFalse(state.selectedUIModuleID.has_value());
    }

    TEST_METHOD(Runtime_FactoryWrongModuleIDFailsActivation)
    {
        RuntimeTestFixture fix;
        TempRuntimeDir dir;
        dir.writeManifest("service.json", makeServiceManifestJSON());

        ModuleRegistry registry;
        registry.registerModule("TestServiceModule", []() -> std::unique_ptr<IForsettiModule> {
            auto desc = makeDescriptor("com.test.other", "Test Service", ModuleType::Service);
            auto manifest = makeManifest(
                "com.test.service", "Test Service", ModuleType::Service, "TestServiceModule");
            return std::make_unique<StubForsettiModule>(desc, manifest);
        });

        auto runtime = ForsettiRuntime(
            fix.makeModuleManager(std::move(registry)),
            fix.entitlements, fix.eventBus, dir.path());

        runtime.boot();

        expectModuleManagerError([&runtime]() {
            runtime.activateModule("com.test.service");
        }, ModuleManagerError::ModuleIdentityMismatch);

        Assert::IsFalse(runtime.moduleManager().isModuleActive("com.test.service"));
        Assert::IsTrue(fix.store->loadState().enabledServiceModuleIDs.empty());
    }

    TEST_METHOD(Runtime_FactoryWrongTypeFailsActivation)
    {
        RuntimeTestFixture fix;
        TempRuntimeDir dir;
        dir.writeManifest("service.json", makeServiceManifestJSON());

        ModuleRegistry registry;
        registry.registerModule("TestServiceModule", []() -> std::unique_ptr<IForsettiModule> {
            auto desc = makeDescriptor("com.test.service", "Test Service", ModuleType::UI);
            auto manifest = makeManifest(
                "com.test.service", "Test Service", ModuleType::Service, "TestServiceModule");
            return std::make_unique<StubForsettiModule>(desc, manifest);
        });

        auto runtime = ForsettiRuntime(
            fix.makeModuleManager(std::move(registry)),
            fix.entitlements, fix.eventBus, dir.path());

        runtime.boot();

        expectModuleManagerError([&runtime]() {
            runtime.activateModule("com.test.service");
        }, ModuleManagerError::ModuleTypeMismatch);

        Assert::IsFalse(runtime.moduleManager().isModuleActive("com.test.service"));
        Assert::IsTrue(fix.store->loadState().enabledServiceModuleIDs.empty());
    }

    TEST_METHOD(Runtime_FactoryWrongVersionFailsActivation)
    {
        RuntimeTestFixture fix;
        TempRuntimeDir dir;
        dir.writeManifest("service.json", makeServiceManifestJSON());

        ModuleRegistry registry;
        registry.registerModule("TestServiceModule", []() -> std::unique_ptr<IForsettiModule> {
            auto desc = makeDescriptor(
                "com.test.service", "Test Service", ModuleType::Service, SemVer{0, 2, 0});
            auto manifest = makeManifest(
                "com.test.service", "Test Service", ModuleType::Service, "TestServiceModule");
            return std::make_unique<StubForsettiModule>(desc, manifest);
        });

        auto runtime = ForsettiRuntime(
            fix.makeModuleManager(std::move(registry)),
            fix.entitlements, fix.eventBus, dir.path());

        runtime.boot();

        expectModuleManagerError([&runtime]() {
            runtime.activateModule("com.test.service");
        }, ModuleManagerError::ModuleVersionMismatch);

        Assert::IsFalse(runtime.moduleManager().isModuleActive("com.test.service"));
        Assert::IsTrue(fix.store->loadState().enabledServiceModuleIDs.empty());
    }

    TEST_METHOD(Runtime_FactoryMismatchedManifestFailsActivation)
    {
        RuntimeTestFixture fix;
        TempRuntimeDir dir;
        dir.writeManifest("service.json", makeServiceManifestJSON());

        ModuleRegistry registry;
        registry.registerModule("TestServiceModule", []() -> std::unique_ptr<IForsettiModule> {
            auto desc = makeDescriptor("com.test.service", "Test Service", ModuleType::Service);
            auto manifest = makeManifest(
                "com.test.service", "Test Service", ModuleType::Service, "DifferentEntryPoint");
            return std::make_unique<StubForsettiModule>(desc, manifest);
        });

        auto runtime = ForsettiRuntime(
            fix.makeModuleManager(std::move(registry)),
            fix.entitlements, fix.eventBus, dir.path());

        runtime.boot();

        expectModuleManagerError([&runtime]() {
            runtime.activateModule("com.test.service");
        }, ModuleManagerError::ModuleManifestMismatch);

        Assert::IsFalse(runtime.moduleManager().isModuleActive("com.test.service"));
        Assert::IsTrue(fix.store->loadState().enabledServiceModuleIDs.empty());
    }

    TEST_METHOD(Runtime_ServiceStartFailureDoesNotPersistActiveState)
    {
        RuntimeTestFixture fix;
        TempRuntimeDir dir;
        dir.writeManifest("service.json", makeServiceManifestJSON());

        ModuleRegistry registry;
        registry.registerModule("TestServiceModule", []() -> std::unique_ptr<IForsettiModule> {
            auto desc = makeDescriptor("com.test.service", "Test Service", ModuleType::Service);
            auto manifest = makeManifest(
                "com.test.service", "Test Service", ModuleType::Service, "TestServiceModule");
            return std::make_unique<ThrowingStartModule>(desc, manifest);
        });

        auto runtime = ForsettiRuntime(
            fix.makeModuleManager(std::move(registry)),
            fix.entitlements, fix.eventBus, dir.path());

        runtime.boot();

        try {
            runtime.activateModule("com.test.service");
            Assert::Fail(L"Expected start failure.");
        } catch (const ModuleManagerException&) {
            Assert::Fail(L"Expected module start failure, not manager validation failure.");
        } catch (const std::runtime_error&) {
        }

        const auto state = fix.store->loadState();
        Assert::IsFalse(runtime.moduleManager().isModuleActive("com.test.service"));
        Assert::IsTrue(state.enabledServiceModuleIDs.empty());
        Assert::IsTrue(state.enabledUIModuleIDs.empty());
        Assert::IsFalse(state.selectedUIModuleID.has_value());
    }

    TEST_METHOD(Runtime_RestoreFailureProducesDiagnosticsAndPersistsReconciledStateOnce)
    {
        RuntimeTestFixture fix;
        fix.entitlements->setUnlocked({"com.test.service", "com.test.missing"});

        ActivationState persisted;
        persisted.enabledServiceModuleIDs = {"com.test.service", "com.test.missing"};
        fix.store->setState(persisted);
        fix.store->resetSaveCount();

        TempRuntimeDir dir;
        dir.writeManifest("service.json", makeServiceManifestJSON());

        ModuleRegistry registry;
        registry.registerModule("TestServiceModule", []() -> std::unique_ptr<IForsettiModule> {
            auto desc = makeDescriptor("com.test.service", "Test Service", ModuleType::Service);
            auto manifest = makeManifest(
                "com.test.service", "Test Service", ModuleType::Service, "TestServiceModule");
            return std::make_unique<StubForsettiModule>(desc, manifest);
        });

        auto runtime = ForsettiRuntime(
            fix.makeModuleManager(std::move(registry)),
            fix.entitlements, fix.eventBus, dir.path());

        runtime.boot();

        const auto result = runtime.moduleManager().lastRestoreResult();
        Assert::AreEqual(size_t(1), result.restoredModuleIDs.size());
        Assert::AreEqual(std::string("com.test.service"), result.restoredModuleIDs[0]);
        Assert::AreEqual(size_t(1), result.failures.size());
        Assert::AreEqual(std::string("com.test.missing"), result.failures[0].moduleID);
        Assert::AreEqual(1, fix.store->saveCount());

        const auto state = fix.store->loadState();
        Assert::IsTrue(state.enabledServiceModuleIDs.contains("com.test.service"));
        Assert::IsFalse(state.enabledServiceModuleIDs.contains("com.test.missing"));

        bool foundDiagnostic = false;
        for (const auto& entry : fix.logger->entries) {
            if (entry.level == LogLevel::Warning &&
                entry.sourceModuleID == "com.test.missing" &&
                entry.message.find("Failed to restore module") != std::string::npos) {
                foundDiagnostic = true;
            }
        }
        Assert::IsTrue(foundDiagnostic);
    }

    TEST_METHOD(Runtime_RestoreHonorsSelectedUIModule)
    {
        RuntimeTestFixture fix;
        fix.entitlements->setUnlocked({"com.test.ui-a", "com.test.ui-b"});

        ActivationState persisted;
        persisted.enabledUIModuleIDs = {"com.test.ui-a", "com.test.ui-b"};
        persisted.selectedUIModuleID = "com.test.ui-a";
        fix.store->setState(persisted);
        fix.store->resetSaveCount();

        TempRuntimeDir dir;
        dir.writeManifest("ui-a.json", makeManifestJSON(
            "com.test.ui-a", "Test UI A", ModuleType::UI, "TestUIModuleA"));
        dir.writeManifest("ui-b.json", makeManifestJSON(
            "com.test.ui-b", "Test UI B", ModuleType::UI, "TestUIModuleB"));

        ModuleRegistry registry;
        registry.registerModule("TestUIModuleA", []() -> std::unique_ptr<IForsettiModule> {
            auto desc = makeDescriptor("com.test.ui-a", "Test UI A", ModuleType::UI);
            auto manifest = makeManifest("com.test.ui-a", "Test UI A", ModuleType::UI, "TestUIModuleA");
            return std::make_unique<StubForsettiUIModule>(desc, manifest);
        });
        registry.registerModule("TestUIModuleB", []() -> std::unique_ptr<IForsettiModule> {
            auto desc = makeDescriptor("com.test.ui-b", "Test UI B", ModuleType::UI);
            auto manifest = makeManifest("com.test.ui-b", "Test UI B", ModuleType::UI, "TestUIModuleB");
            return std::make_unique<StubForsettiUIModule>(desc, manifest);
        });

        auto runtime = ForsettiRuntime(
            fix.makeModuleManager(std::move(registry)),
            fix.entitlements, fix.eventBus, dir.path());

        runtime.boot();

        Assert::IsTrue(runtime.moduleManager().activeUIModuleID().has_value());
        Assert::AreEqual(
            std::string("com.test.ui-a"),
            runtime.moduleManager().activeUIModuleID().value());
        Assert::AreEqual(1, fix.store->saveCount());

        const auto state = fix.store->loadState();
        Assert::IsTrue(state.enabledUIModuleIDs.contains("com.test.ui-a"));
        Assert::IsFalse(state.enabledUIModuleIDs.contains("com.test.ui-b"));
        Assert::IsTrue(state.selectedUIModuleID.has_value());
        Assert::AreEqual(std::string("com.test.ui-a"), state.selectedUIModuleID.value());
    }
};
