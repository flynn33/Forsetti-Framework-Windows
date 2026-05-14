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
        const std::string& entryPoint,
        const std::vector<Capability>& capabilities = {}) {
        auto capabilitiesJson = nlohmann::json::array();
        for (const auto capability : capabilities) {
            capabilitiesJson.push_back(Forsetti::to_string(capability));
        }

        return nlohmann::json{
            {"schemaVersion", "1.0"},
            {"moduleID", moduleID},
            {"displayName", displayName},
            {"moduleVersion", {{"major", 0}, {"minor", 1}, {"patch", 0}, {"prerelease", nullptr}}},
            {"moduleType", Forsetti::to_string(moduleType)},
            {"supportedPlatforms", nlohmann::json::array({"Windows"})},
            {"minForsettiVersion", {{"major", 0}, {"minor", 1}, {"patch", 0}, {"prerelease", nullptr}}},
            {"maxForsettiVersion", nullptr},
            {"capabilitiesRequested", std::move(capabilitiesJson)},
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
        SemVer version = SemVer{0, 1, 0},
        std::vector<Capability> capabilities = {}) {
        return ModuleManifest{
            .schemaVersion = "1.0",
            .moduleID = moduleID,
            .displayName = displayName,
            .moduleVersion = version,
            .moduleType = moduleType,
            .supportedPlatforms = {Platform::Windows},
            .minForsettiVersion = SemVer{0, 1, 0},
            .maxForsettiVersion = std::nullopt,
            .capabilitiesRequested = std::move(capabilities),
            .iapProductID = std::nullopt,
            .entryPoint = entryPoint
        };
    }

    UIContributions makeToolbarContributions(const std::string& itemID) {
        UIContributions contributions;
        contributions.toolbarItems.push_back(ToolbarItemDescriptor{
            .itemID = itemID,
            .title = itemID,
            .systemImageName = "",
            .action = NavigateAction{.destinationID = "home"}
        });
        return contributions;
    }

    UIContributions makeOverlayContributions(const std::string& routeID) {
        UIContributions contributions;
        OverlaySchema overlay;
        overlay.navigationPointers.push_back(NavigationPointer{
            .pointerID = routeID + "-pointer",
            .label = routeID,
            .baseDestinationID = "home"
        });
        overlay.overlayRoutes.push_back(OverlayRoute{
            .routeID = routeID,
            .label = routeID,
            .presentation = OverlayPresentation::Sheet,
            .destination = BaseOverlayDestination{.destinationID = "home"}
        });
        contributions.overlaySchema = std::move(overlay);
        return contributions;
    }

    UIContributions makeViewInjectionContributions(const std::string& injectionID) {
        UIContributions contributions;
        contributions.viewInjections.push_back(ViewInjectionDescriptor{
            .injectionID = injectionID,
            .slot = "homeBanner",
            .viewID = "ExampleView",
            .priority = 10
        });
        return contributions;
    }

    UIContributions makePublishingToolbarContributions(const std::string& itemID) {
        UIContributions contributions;
        contributions.toolbarItems.push_back(ToolbarItemDescriptor{
            .itemID = itemID,
            .title = itemID,
            .systemImageName = "",
            .action = PublishEventAction{.eventType = "custom.event"}
        });
        return contributions;
    }

    struct TrackingUIModuleState {
        bool started = false;
        int startCount = 0;
        int stopCount = 0;
    };

    class TrackingUIModule final : public IForsettiUIModule {
        ModuleDescriptor desc_;
        ModuleManifest manifest_;
        UIContributions contributions_;
        std::shared_ptr<TrackingUIModuleState> state_;
        bool throwOnStart_ = false;
    public:
        TrackingUIModule(
            ModuleDescriptor desc,
            ModuleManifest manifest,
            UIContributions contributions,
            std::shared_ptr<TrackingUIModuleState> state,
            bool throwOnStart = false)
            : desc_(std::move(desc))
            , manifest_(std::move(manifest))
            , contributions_(std::move(contributions))
            , state_(std::move(state))
            , throwOnStart_(throwOnStart)
        {
        }

        ModuleDescriptor descriptor() const override { return desc_; }
        ModuleManifest manifest() const override { return manifest_; }

        void start(ForsettiContext& /*ctx*/) override {
            ++state_->startCount;
            state_->started = true;
            if (throwOnStart_) {
                throw std::runtime_error("ui start failed");
            }
        }

        void stop(ForsettiContext& /*ctx*/) override {
            ++state_->stopCount;
            state_->started = false;
        }

        UIContributions uiContributions() const override { return contributions_; }
    };

    void registerTrackingUIModule(
        ModuleRegistry& registry,
        const std::string& entryPoint,
        const std::string& moduleID,
        const std::string& displayName,
        std::vector<Capability> capabilities,
        UIContributions contributions,
        std::shared_ptr<TrackingUIModuleState> state,
        bool throwOnStart = false) {
        registry.registerModule(
            entryPoint,
            [entryPoint, moduleID, displayName, capabilities = std::move(capabilities),
             contributions = std::move(contributions), state = std::move(state), throwOnStart]()
                -> std::unique_ptr<IForsettiModule> {
                auto desc = makeDescriptor(moduleID, displayName, ModuleType::UI);
                auto manifest = makeManifest(
                    moduleID,
                    displayName,
                    ModuleType::UI,
                    entryPoint,
                    SemVer{0, 1, 0},
                    capabilities);
                return std::make_unique<TrackingUIModule>(
                    desc, manifest, contributions, state, throwOnStart);
            });
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

    std::string expectModuleManagerErrorMessage(
        const std::function<void()>& action,
        ModuleManagerError expectedError) {
        std::string message;
        bool caughtExpectedException = false;

        try {
            action();
        } catch (const ModuleManagerException& ex) {
            caughtExpectedException = true;
            Assert::IsTrue(ex.error() == expectedError);
            message = ex.what();
        }

        if (!caughtExpectedException) {
            Assert::Fail(L"Expected ModuleManagerException.");
        }

        return message;
    }

    void assertContains(const std::string& text, const std::string& expected) {
        Assert::IsTrue(text.find(expected) != std::string::npos);
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

        const auto message = expectModuleManagerErrorMessage([&runtime]() {
            runtime.activateModule("com.test.service");
        }, ModuleManagerError::ModuleIdentityMismatch);

        assertContains(message, "expected manifest.moduleID \"com.test.service\"");
        assertContains(message, "actual descriptor.moduleID \"com.test.other\"");
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

        const auto message = expectModuleManagerErrorMessage([&runtime]() {
            runtime.activateModule("com.test.service");
        }, ModuleManagerError::ModuleTypeMismatch);

        assertContains(message, "expected manifest.moduleType \"service\"");
        assertContains(message, "actual descriptor.type \"ui\"");
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

        const auto message = expectModuleManagerErrorMessage([&runtime]() {
            runtime.activateModule("com.test.service");
        }, ModuleManagerError::ModuleVersionMismatch);

        assertContains(message, "expected manifest.moduleVersion \"0.1.0\"");
        assertContains(message, "actual descriptor.version \"0.2.0\"");
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

        const auto message = expectModuleManagerErrorMessage([&runtime]() {
            runtime.activateModule("com.test.service");
        }, ModuleManagerError::ModuleManifestMismatch);

        assertContains(message, "entryPoint expected manifest.entryPoint \"TestServiceModule\"");
        assertContains(message, "actual bundled.entryPoint \"DifferentEntryPoint\"");
        Assert::IsFalse(runtime.moduleManager().isModuleActive("com.test.service"));
        Assert::IsTrue(fix.store->loadState().enabledServiceModuleIDs.empty());
    }

    TEST_METHOD(Runtime_UISurfaceCallbackFailurePreservesPreviousUIModule)
    {
        RuntimeTestFixture fix;
        fix.entitlements->setUnlocked({"com.test.ui-a", "com.test.ui-b"});

        TempRuntimeDir dir;
        dir.writeManifest("ui-a.json", makeManifestJSON(
            "com.test.ui-a", "Test UI A", ModuleType::UI, "TestUIModuleA",
            {Capability::ToolbarItems}));
        dir.writeManifest("ui-b.json", makeManifestJSON(
            "com.test.ui-b", "Test UI B", ModuleType::UI, "TestUIModuleB",
            {Capability::ToolbarItems}));

        auto stateA = std::make_shared<TrackingUIModuleState>();
        auto stateB = std::make_shared<TrackingUIModuleState>();

        ModuleRegistry registry;
        registry.registerModule("TestUIModuleA", [stateA]() -> std::unique_ptr<IForsettiModule> {
            auto desc = makeDescriptor("com.test.ui-a", "Test UI A", ModuleType::UI);
            auto manifest = makeManifest(
                "com.test.ui-a", "Test UI A", ModuleType::UI, "TestUIModuleA",
                SemVer{0, 1, 0}, {Capability::ToolbarItems});
            return std::make_unique<TrackingUIModule>(
                desc, manifest, makeToolbarContributions("ui-a-toolbar"), stateA);
        });
        registry.registerModule("TestUIModuleB", [stateB]() -> std::unique_ptr<IForsettiModule> {
            auto desc = makeDescriptor("com.test.ui-b", "Test UI B", ModuleType::UI);
            auto manifest = makeManifest(
                "com.test.ui-b", "Test UI B", ModuleType::UI, "TestUIModuleB",
                SemVer{0, 1, 0}, {Capability::ToolbarItems});
            return std::make_unique<TrackingUIModule>(
                desc, manifest, makeToolbarContributions("ui-b-toolbar"), stateB);
        });

        auto runtime = ForsettiRuntime(
            fix.makeModuleManager(std::move(registry)),
            fix.entitlements, fix.eventBus, dir.path());

        runtime.boot();
        runtime.activateModule("com.test.ui-a");

        fix.surfaceManager->onChanged([]() {
            throw std::runtime_error("surface callback failed");
        });

        try {
            runtime.activateModule("com.test.ui-b");
            Assert::Fail(L"Expected surface callback failure.");
        } catch (const std::runtime_error&) {
        }

        Assert::AreEqual(std::string("com.test.ui-a"), runtime.moduleManager().activeUIModuleID().value());
        Assert::IsTrue(runtime.moduleManager().isModuleActive("com.test.ui-a"));
        Assert::IsFalse(runtime.moduleManager().isModuleActive("com.test.ui-b"));
        Assert::IsTrue(stateA->started);
        Assert::AreEqual(1, stateA->startCount);
        Assert::AreEqual(0, stateA->stopCount);
        Assert::IsFalse(stateB->started);
        Assert::AreEqual(0, stateB->startCount);

        const auto& toolbarItems = fix.surfaceManager->currentToolbarItems();
        Assert::AreEqual(size_t(1), toolbarItems.size());
        Assert::AreEqual(std::string("ui-a-toolbar"), toolbarItems[0].itemID);
    }

    TEST_METHOD(Runtime_UIStartFailureRestoresPreviousSurfaceAndStopsIncoming)
    {
        RuntimeTestFixture fix;
        fix.entitlements->setUnlocked({"com.test.ui-a", "com.test.ui-b"});

        TempRuntimeDir dir;
        dir.writeManifest("ui-a.json", makeManifestJSON(
            "com.test.ui-a", "Test UI A", ModuleType::UI, "TestUIModuleA",
            {Capability::ToolbarItems}));
        dir.writeManifest("ui-b.json", makeManifestJSON(
            "com.test.ui-b", "Test UI B", ModuleType::UI, "TestUIModuleB",
            {Capability::ToolbarItems}));

        auto stateA = std::make_shared<TrackingUIModuleState>();
        auto stateB = std::make_shared<TrackingUIModuleState>();

        ModuleRegistry registry;
        registry.registerModule("TestUIModuleA", [stateA]() -> std::unique_ptr<IForsettiModule> {
            auto desc = makeDescriptor("com.test.ui-a", "Test UI A", ModuleType::UI);
            auto manifest = makeManifest(
                "com.test.ui-a", "Test UI A", ModuleType::UI, "TestUIModuleA",
                SemVer{0, 1, 0}, {Capability::ToolbarItems});
            return std::make_unique<TrackingUIModule>(
                desc, manifest, makeToolbarContributions("ui-a-toolbar"), stateA);
        });
        registry.registerModule("TestUIModuleB", [stateB]() -> std::unique_ptr<IForsettiModule> {
            auto desc = makeDescriptor("com.test.ui-b", "Test UI B", ModuleType::UI);
            auto manifest = makeManifest(
                "com.test.ui-b", "Test UI B", ModuleType::UI, "TestUIModuleB",
                SemVer{0, 1, 0}, {Capability::ToolbarItems});
            return std::make_unique<TrackingUIModule>(
                desc, manifest, makeToolbarContributions("ui-b-toolbar"), stateB, true);
        });

        auto runtime = ForsettiRuntime(
            fix.makeModuleManager(std::move(registry)),
            fix.entitlements, fix.eventBus, dir.path());

        runtime.boot();
        runtime.activateModule("com.test.ui-a");

        try {
            runtime.activateModule("com.test.ui-b");
            Assert::Fail(L"Expected UI start failure.");
        } catch (const std::runtime_error&) {
        }

        Assert::AreEqual(std::string("com.test.ui-a"), runtime.moduleManager().activeUIModuleID().value());
        Assert::IsTrue(runtime.moduleManager().isModuleActive("com.test.ui-a"));
        Assert::IsFalse(runtime.moduleManager().isModuleActive("com.test.ui-b"));
        Assert::IsTrue(stateA->started);
        Assert::AreEqual(1, stateA->startCount);
        Assert::AreEqual(0, stateA->stopCount);
        Assert::IsFalse(stateB->started);
        Assert::AreEqual(1, stateB->startCount);
        Assert::AreEqual(1, stateB->stopCount);

        const auto& toolbarItems = fix.surfaceManager->currentToolbarItems();
        Assert::AreEqual(size_t(1), toolbarItems.size());
        Assert::AreEqual(std::string("ui-a-toolbar"), toolbarItems[0].itemID);
    }

    TEST_METHOD(Runtime_UIToolbarContributionRequiresToolbarCapability)
    {
        RuntimeTestFixture fix;
        fix.entitlements->setUnlocked({"com.test.ui"});

        TempRuntimeDir dir;
        dir.writeManifest("ui.json", makeManifestJSON(
            "com.test.ui", "Test UI", ModuleType::UI, "TestUIModule"));

        auto state = std::make_shared<TrackingUIModuleState>();
        ModuleRegistry registry;
        registerTrackingUIModule(
            registry,
            "TestUIModule",
            "com.test.ui",
            "Test UI",
            {},
            makeToolbarContributions("ui-toolbar"),
            state);

        auto runtime = ForsettiRuntime(
            fix.makeModuleManager(std::move(registry)),
            fix.entitlements, fix.eventBus, dir.path());

        runtime.boot();

        const auto message = expectModuleManagerErrorMessage([&runtime]() {
            runtime.activateModule("com.test.ui");
        }, ModuleManagerError::CapabilityDenied);

        assertContains(message, "toolbar_items");
        Assert::IsFalse(runtime.moduleManager().isModuleActive("com.test.ui"));
        Assert::IsFalse(state->started);
        Assert::AreEqual(size_t(0), fix.surfaceManager->currentToolbarItems().size());
    }

    TEST_METHOD(Runtime_UIViewContributionRequiresViewCapability)
    {
        RuntimeTestFixture fix;
        fix.entitlements->setUnlocked({"com.test.ui"});

        TempRuntimeDir dir;
        dir.writeManifest("ui.json", makeManifestJSON(
            "com.test.ui", "Test UI", ModuleType::UI, "TestUIModule"));

        auto state = std::make_shared<TrackingUIModuleState>();
        ModuleRegistry registry;
        registerTrackingUIModule(
            registry,
            "TestUIModule",
            "com.test.ui",
            "Test UI",
            {},
            makeViewInjectionContributions("ui-view"),
            state);

        auto runtime = ForsettiRuntime(
            fix.makeModuleManager(std::move(registry)),
            fix.entitlements, fix.eventBus, dir.path());

        runtime.boot();

        const auto message = expectModuleManagerErrorMessage([&runtime]() {
            runtime.activateModule("com.test.ui");
        }, ModuleManagerError::CapabilityDenied);

        assertContains(message, "view_injection");
        Assert::IsFalse(runtime.moduleManager().isModuleActive("com.test.ui"));
        Assert::IsFalse(state->started);
        Assert::AreEqual(size_t(0), fix.surfaceManager->currentViewInjectionsBySlot().size());
    }

    TEST_METHOD(Runtime_UIOverlayContributionRequiresRoutingCapability)
    {
        RuntimeTestFixture fix;
        fix.entitlements->setUnlocked({"com.test.ui"});

        TempRuntimeDir dir;
        dir.writeManifest("ui.json", makeManifestJSON(
            "com.test.ui", "Test UI", ModuleType::UI, "TestUIModule"));

        auto state = std::make_shared<TrackingUIModuleState>();
        ModuleRegistry registry;
        registerTrackingUIModule(
            registry,
            "TestUIModule",
            "com.test.ui",
            "Test UI",
            {},
            makeOverlayContributions("ui-route"),
            state);

        auto runtime = ForsettiRuntime(
            fix.makeModuleManager(std::move(registry)),
            fix.entitlements, fix.eventBus, dir.path());

        runtime.boot();

        const auto message = expectModuleManagerErrorMessage([&runtime]() {
            runtime.activateModule("com.test.ui");
        }, ModuleManagerError::CapabilityDenied);

        assertContains(message, "routing_overlay");
        Assert::IsFalse(runtime.moduleManager().isModuleActive("com.test.ui"));
        Assert::IsFalse(state->started);
        Assert::IsFalse(fix.surfaceManager->currentOverlaySchema().has_value());
    }

    TEST_METHOD(Runtime_UIPublishingToolbarActionRequiresEventCapability)
    {
        RuntimeTestFixture fix;
        fix.entitlements->setUnlocked({"com.test.ui"});

        TempRuntimeDir dir;
        dir.writeManifest("ui.json", makeManifestJSON(
            "com.test.ui", "Test UI", ModuleType::UI, "TestUIModule",
            {Capability::ToolbarItems}));

        auto state = std::make_shared<TrackingUIModuleState>();
        ModuleRegistry registry;
        registerTrackingUIModule(
            registry,
            "TestUIModule",
            "com.test.ui",
            "Test UI",
            {Capability::ToolbarItems},
            makePublishingToolbarContributions("ui-event-toolbar"),
            state);

        auto runtime = ForsettiRuntime(
            fix.makeModuleManager(std::move(registry)),
            fix.entitlements, fix.eventBus, dir.path());

        runtime.boot();

        const auto message = expectModuleManagerErrorMessage([&runtime]() {
            runtime.activateModule("com.test.ui");
        }, ModuleManagerError::CapabilityDenied);

        assertContains(message, "event_publishing");
        Assert::IsFalse(runtime.moduleManager().isModuleActive("com.test.ui"));
        Assert::IsFalse(state->started);
        Assert::AreEqual(size_t(0), fix.surfaceManager->currentToolbarItems().size());
    }

    TEST_METHOD(Runtime_UISwitchReplacesActiveModuleAndSurface)
    {
        RuntimeTestFixture fix;
        fix.entitlements->setUnlocked({"com.test.ui-a", "com.test.ui-b"});

        TempRuntimeDir dir;
        dir.writeManifest("ui-a.json", makeManifestJSON(
            "com.test.ui-a", "Test UI A", ModuleType::UI, "TestUIModuleA",
            {Capability::ToolbarItems}));
        dir.writeManifest("ui-b.json", makeManifestJSON(
            "com.test.ui-b", "Test UI B", ModuleType::UI, "TestUIModuleB",
            {Capability::ToolbarItems}));

        auto stateA = std::make_shared<TrackingUIModuleState>();
        auto stateB = std::make_shared<TrackingUIModuleState>();

        ModuleRegistry registry;
        registerTrackingUIModule(
            registry,
            "TestUIModuleA",
            "com.test.ui-a",
            "Test UI A",
            {Capability::ToolbarItems},
            makeToolbarContributions("ui-a-toolbar"),
            stateA);
        registerTrackingUIModule(
            registry,
            "TestUIModuleB",
            "com.test.ui-b",
            "Test UI B",
            {Capability::ToolbarItems},
            makeToolbarContributions("ui-b-toolbar"),
            stateB);

        auto runtime = ForsettiRuntime(
            fix.makeModuleManager(std::move(registry)),
            fix.entitlements, fix.eventBus, dir.path());

        runtime.boot();
        runtime.activateModule("com.test.ui-a");
        runtime.activateModule("com.test.ui-b");

        Assert::IsTrue(runtime.moduleManager().activeUIModuleID().has_value());
        Assert::AreEqual(
            std::string("com.test.ui-b"),
            runtime.moduleManager().activeUIModuleID().value());
        Assert::IsFalse(runtime.moduleManager().isModuleActive("com.test.ui-a"));
        Assert::IsTrue(runtime.moduleManager().isModuleActive("com.test.ui-b"));
        Assert::IsFalse(stateA->started);
        Assert::AreEqual(1, stateA->stopCount);
        Assert::IsTrue(stateB->started);

        const auto& toolbarItems = fix.surfaceManager->currentToolbarItems();
        Assert::AreEqual(size_t(1), toolbarItems.size());
        Assert::AreEqual(std::string("ui-b-toolbar"), toolbarItems[0].itemID);

        const auto state = fix.store->loadState();
        Assert::IsFalse(state.enabledUIModuleIDs.contains("com.test.ui-a"));
        Assert::IsTrue(state.enabledUIModuleIDs.contains("com.test.ui-b"));
        Assert::AreEqual(std::string("com.test.ui-b"), state.selectedUIModuleID.value());
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
