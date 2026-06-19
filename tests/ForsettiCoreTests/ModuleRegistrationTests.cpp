// Forsetti Framework for Windows
// Copyright (c) 2026 James Daley. All Rights Reserved.
// Proprietary and Confidential. Patent Pending.

#include "CppUnitTest.h"
#include "ForsettiCore/ModuleRegistration.h"

#include <memory>
#include <stdexcept>
#include <utility>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Forsetti;

namespace {

class FixedRegistrationClock final : public IRegistrationClock {
public:
    explicit FixedRegistrationClock(std::string value)
        : value_(std::move(value)) {}

    std::string now() const override { return value_; }

private:
    std::string value_;
};

ModuleManifest makeRegistrationManifest(const std::string& moduleID = "com.test.registration")
{
    return ModuleManifest{
        .schemaVersion = "1.1",
        .moduleID = moduleID,
        .displayName = "Registration Test Module",
        .moduleVersion = SemVer{1, 0, 0},
        .moduleType = ModuleType::Service,
        .supportedPlatforms = {Platform::Windows},
        .minForsettiVersion = SemVer{0, 2, 0},
        .maxForsettiVersion = std::nullopt,
        .capabilitiesRequested = {Capability::Storage},
        .iapProductID = std::nullopt,
        .entryPoint = "RegistrationTestModule",
        .manifestTemplateVersion = ManifestTemplateVersion::V1_1,
        .defaultModuleRole = std::nullopt,
        .runtimeRequirements = ModuleRuntimeRequirements{
            .io = {
                ModuleIORequirement{
                    .requirementID = "storage.registration-state",
                    .kind = ModuleIOKind::Storage,
                    .access = ModuleIOAccess::ReadWrite,
                    .required = true,
                    .description = std::nullopt
                }
            },
            .ui = std::nullopt,
            .dataIsolation = ModuleDataIsolation{
                .mode = ModuleDataIsolationMode::PrivateToModule,
                .ownedStoreIDs = {"registration-state"},
                .requiredDefaultRoles = {}
            }
        }
    };
}

std::shared_ptr<ModuleRegistrationService> makeRegistrationService()
{
    return std::make_shared<ModuleRegistrationService>(
        std::make_shared<InMemoryModuleRegistrationStore>(),
        std::make_shared<Sha256ManifestDigestProvider>(),
        std::make_shared<FixedRegistrationClock>("1000"));
}

} // namespace

TEST_CLASS(ModuleRegistrationTests)
{
public:

    TEST_METHOD(InMemoryStore_CRUD)
    {
        InMemoryModuleRegistrationStore store;
        auto manifest = makeRegistrationManifest();
        Sha256ManifestDigestProvider digestProvider;

        ModuleRegistrationRecord record{
            .moduleID = manifest.moduleID,
            .displayName = manifest.displayName,
            .moduleVersion = manifest.moduleVersion,
            .moduleType = manifest.moduleType,
            .entryPoint = manifest.entryPoint,
            .schemaVersion = manifest.schemaVersion,
            .manifestTemplateVersion = manifest.manifestTemplateVersion,
            .canonicalManifestHash = digestProvider.digestManifest(manifest),
            .supportedPlatforms = manifest.supportedPlatforms,
            .capabilitiesRequested = manifest.capabilitiesRequested,
            .defaultModuleRole = manifest.defaultModuleRole,
            .runtimeRequirements = manifest.runtimeRequirements,
            .registeredAt = "1",
            .lastConfirmedAt = "2",
            .confirmed = true
        };

        store.save(record);
        auto loaded = store.load(record.moduleID);
        Assert::IsTrue(loaded.has_value());
        Assert::IsTrue(loaded.value() == record);
        Assert::AreEqual(size_t(1), store.loadAll().size());

        store.remove(record.moduleID);
        Assert::IsFalse(store.load(record.moduleID).has_value());
    }

    TEST_METHOD(CanonicalManifestDigest_IsStable)
    {
        Sha256ManifestDigestProvider digestProvider;
        const auto manifest = makeRegistrationManifest();

        const auto first = digestProvider.digestManifest(manifest);
        const auto second = digestProvider.digestManifest(manifest);

        Assert::AreEqual(first, second);
        Assert::AreEqual(size_t(64), first.size());
    }

    TEST_METHOD(CanonicalManifestDigest_ChangesForSemanticChange)
    {
        Sha256ManifestDigestProvider digestProvider;
        auto firstManifest = makeRegistrationManifest();
        auto secondManifest = makeRegistrationManifest();
        secondManifest.runtimeRequirements.io[0].required = false;

        Assert::AreNotEqual(
            digestProvider.digestManifest(firstManifest),
            digestProvider.digestManifest(secondManifest));
    }

    TEST_METHOD(RegisterAndConfirmManifest)
    {
        auto service = makeRegistrationService();
        auto manifest = makeRegistrationManifest();

        auto registered = service->registerDiscoveredManifest(manifest);
        Assert::IsFalse(registered.confirmed);
        Assert::IsFalse(service->isConfirmedMatch(manifest));

        auto confirmed = service->confirmDiscoveredManifest(manifest);
        Assert::IsTrue(confirmed.confirmed);
        Assert::AreEqual(std::string("1000"), confirmed.lastConfirmedAt);
        Assert::IsTrue(service->isConfirmedMatch(manifest));
    }

    TEST_METHOD(ConfirmRejectsMismatchedRecord)
    {
        auto service = makeRegistrationService();
        auto manifest = makeRegistrationManifest();
        service->confirmDiscoveredManifest(manifest);

        auto changed = manifest;
        changed.displayName = "Changed Registration Test Module";

        Assert::ExpectException<std::runtime_error>([&service, &changed]() {
            (void)service->confirmDiscoveredManifest(changed);
        });
    }

    TEST_METHOD(ConfirmAllReconcilesRemovedManifests)
    {
        auto service = makeRegistrationService();
        auto first = makeRegistrationManifest("com.test.registration.one");
        auto second = makeRegistrationManifest("com.test.registration.two");

        service->confirmAllDiscoveredManifests({first, second});
        Assert::AreEqual(size_t(2), service->registeredModules().size());

        service->confirmAllDiscoveredManifests({first});
        Assert::AreEqual(size_t(1), service->registeredModules().size());
        Assert::IsTrue(service->load(first.moduleID).has_value());
        Assert::IsFalse(service->load(second.moduleID).has_value());
    }
};
