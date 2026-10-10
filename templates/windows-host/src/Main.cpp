// Forsetti Framework for Windows
// Copyright (c) 2026 James Daley.
// SPDX-License-Identifier: Apache-2.0

#include "ActivationStore.h"
#include "NativePresentation.h"
#include "TemplateAppModule.h"
#include "TemplateUIModule.h"
#include "TemplateServiceModule.h"
#include "ForsettiCore/ManifestLoader.h"
#include "ForsettiHostTemplate/ForsettiHostBootstrap.h"
#include "ForsettiPlatform/DefaultPlatformServices.h"
#include <Windows.h>
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
std::filesystem::path executableDirectory()
{
    std::vector<wchar_t> buffer(32768);
    const auto length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (!length || length >= buffer.size()) { throw std::runtime_error("Cannot locate application executable."); }
    return std::filesystem::path(std::wstring(buffer.data(), length)).parent_path();
}
std::filesystem::path statePath()
{
    wchar_t buffer[32768]{};
    const auto length = GetEnvironmentVariableW(L"LOCALAPPDATA", buffer, 32768);
    if (!length || length >= 32768) { throw std::runtime_error("LOCALAPPDATA is unavailable."); }
    return std::filesystem::path(buffer) / L"TemplateHost" / L"activation.json";
}
void require(bool condition, const char* message)
{
    if (!condition) { throw std::runtime_error(message); }
}
class VerificationStateDirectory final {
public:
    VerificationStateDirectory()
    {
        std::vector<wchar_t> buffer(32768);
        const auto length = GetTempPathW(static_cast<DWORD>(buffer.size()), buffer.data());
        if (!length || length >= buffer.size()) { throw std::runtime_error("Cannot locate test temporary directory."); }
        const auto name = L"Forsetti consumer state 日本語_" + std::to_wstring(GetCurrentProcessId()) +
            L"_" + std::to_wstring(GetTickCount64());
        directory_ = std::filesystem::path(std::wstring(buffer.data(), length)) / name;
        if (!CreateDirectoryW(directory_.c_str(), nullptr)) {
            throw std::runtime_error("Cannot create an exclusive consumer-state test directory.");
        }
    }
    VerificationStateDirectory(const VerificationStateDirectory&) = delete;
    VerificationStateDirectory& operator=(const VerificationStateDirectory&) = delete;
    ~VerificationStateDirectory()
    {
        // Remove only owned files; never recursively remove a temporary directory.
        DeleteFileW((directory_ / L"activation.json").c_str());
        DeleteFileW((directory_ / L"activation.json.tmp").c_str());
        RemoveDirectoryW(directory_.c_str());
    }
    std::filesystem::path stateFile() const { return directory_ / L"activation.json"; }
private:
    std::filesystem::path directory_;
};
}

int main(int argc, char* argv[])
{
    try {
        const bool verify = argc == 2 && std::string(argv[1]) == "--verify";
        if (argc > 1 && !verify) { throw std::invalid_argument("Usage: TemplateHost [--verify]"); }
        const auto directory = executableDirectory() / "ForsettiManifests";
        const auto manifests = Forsetti::ManifestLoader::loadManifests(directory.string());
        std::map<std::string, Forsetti::ModuleManifest> catalog;
        for (const auto& manifest : manifests) { catalog.emplace(manifest.moduleID, manifest); }
        const auto appManifest = catalog.at("com.forsetti.template.app");
        const auto uiManifest = catalog.at("com.forsetti.template.ui");
        const auto serviceManifest = catalog.at("com.forsetti.template.service");
        Forsetti::ModuleRegistry registry;
        registry.registerModule("TemplateAppModule", [appManifest]() {
            return std::make_unique<TemplateAppModule>(appManifest);
        });
        registry.registerModule("TemplateUIModule", [uiManifest]() {
            return std::make_unique<TemplateUIModule>(uiManifest);
        });
        registry.registerModule("TemplateServiceModule", [serviceManifest]() {
            return std::make_unique<TemplateServiceModule>(serviceManifest);
        });
        auto services = std::make_shared<Forsetti::ServiceContainer>();
        Forsetti::DefaultForsettiPlatformServices::registerAll(*services);
        auto entitlements = std::make_shared<Forsetti::StaticEntitlementProvider>();
        entitlements->setUnlockedModules(verify ? std::set<std::string>{serviceManifest.moduleID} :
            std::set<std::string>{appManifest.moduleID, uiManifest.moduleID, serviceManifest.moduleID});
        auto surface = std::make_shared<Forsetti::UISurfaceManager>();
        auto registration = Forsetti::ModuleRegistrationService::makeInMemory();
        auto eventBus = std::make_shared<Forsetti::InMemoryEventBus>();
        int appStarted = 0;
        int appStopped = 0;
        auto startedToken = eventBus->makeToken(eventBus->subscribe("com.forsetti.template.app.started",
            [&appStarted, &appManifest](const Forsetti::ForsettiEvent& event) {
                require(event.sourceModuleID == appManifest.moduleID, "Incorrect event source.");
                ++appStarted;
            }));
        auto stoppedToken = eventBus->makeToken(eventBus->subscribe("com.forsetti.template.app.stopped",
            [&appStopped](const Forsetti::ForsettiEvent&) { ++appStopped; }));
        std::optional<VerificationStateDirectory> verificationDirectory;
        if (verify) { verificationDirectory.emplace(); }
        const auto activationPath = verify ? verificationDirectory->stateFile() : statePath();
        auto activationStore = std::make_shared<FileActivationStore>(activationPath);
        if (verify) { require(activationStore->loadState() == Forsetti::ActivationState{}, "Initial disk state is not empty."); }
        Forsetti::ForsettiHostBootstrapConfiguration configuration;
        configuration.manifestDirectory = directory.string();
        configuration.moduleRegistry = std::move(registry);
        configuration.activationStore = activationStore;
        configuration.entitlementProvider = entitlements;
        configuration.services = services;
        configuration.eventBus = eventBus;
        configuration.surfaceManager = surface;
        configuration.registrationService = registration;
        auto controller = Forsetti::ForsettiHostBootstrap{}.makeController(std::move(configuration));
        controller->boot(Forsetti::ForsettiHostLaunchStrategy::restoreOnly());
        auto surfaceAdapter = std::make_shared<Forsetti::ForsettiSurfaceStateAdapter>(surface);
        NativePresentation presentation(controller, surfaceAdapter, manifests);
        if (verify) {
            require(controller->snapshot().activeModuleIDs.empty(), "Restore-only boot activated new modules.");
            require(registration->registeredModules().size() == 3, "Discovery did not register all modules.");
            bool denied = false;
            try { controller->activateModule(appManifest.moduleID); }
            catch (const Forsetti::ModuleManagerException& error) {
                denied = error.error() == Forsetti::ModuleManagerError::EntitlementRequired;
            }
            require(denied && controller->snapshot().activeModuleIDs.empty(), "Denied activation changed state.");
            require(appStarted == 0, "Denied activation started the module.");
            entitlements->setUnlockedModules({appManifest.moduleID, uiManifest.moduleID, serviceManifest.moduleID});
            controller->activateModule(serviceManifest.moduleID);
            controller->activateModule(appManifest.moduleID);
            require(appStarted == 1, "App did not publish its framework-mediated lifecycle event.");
            presentation.render();
            require(presentation.hasPresentedView(appManifest.runtimeRequirements.ui->viewIDs.front()),
                "App native presentation was not created and visible.");
            controller->activateModule(uiManifest.moduleID);
            const auto state = controller->snapshot();
            require(state.activeUIModuleID == uiManifest.moduleID && state.activeModuleIDs.size() == 2 &&
                std::find(state.activeModuleIDs.begin(), state.activeModuleIDs.end(), appManifest.moduleID) ==
                    state.activeModuleIDs.end(), "UI/app shared active slot was not enforced.");
            require(appStopped == 1, "Replaced app was not stopped.");
            presentation.render();
            require(presentation.hasPresentedView(uiManifest.runtimeRequirements.ui->viewIDs.front()),
                "UI native presentation was not created and visible.");
            const auto desiredState = activationStore->loadState();
            require(desiredState.enabledServiceModuleIDs == std::set<std::string>{serviceManifest.moduleID} &&
                desiredState.enabledUIModuleIDs == std::set<std::string>{uiManifest.moduleID} &&
                desiredState.selectedUIModuleID == uiManifest.moduleID,
                "Service/UI selection was not persisted to the app-owned state file.");
            controller->shutdown();
            require(controller->snapshot().activeModuleIDs.empty(), "Shutdown left active modules.");
            require(activationStore->loadState() == desiredState, "Shutdown changed the desired disk activation state.");
            auto reopenedStore = std::make_shared<FileActivationStore>(activationPath);
            require(reopenedStore->loadState() == desiredState, "FileActivationStore disk roundtrip changed state.");
            controller->boot(Forsetti::ForsettiHostLaunchStrategy::restoreOnly());
            const auto restored = controller->snapshot();
            require(restored.activeUIModuleID == uiManifest.moduleID && restored.activeModuleIDs.size() == 2 &&
                std::find(restored.activeModuleIDs.begin(), restored.activeModuleIDs.end(), serviceManifest.moduleID) !=
                    restored.activeModuleIDs.end(), "Restore-only reboot did not restore the service/UI selection.");
            presentation.render();
            require(presentation.hasPresentedView(uiManifest.runtimeRequirements.ui->viewIDs.front()),
                "Restored UI native presentation was not created and visible.");
            controller->shutdown();
            require(controller->snapshot().activeModuleIDs.empty() && activationStore->loadState() == desiredState,
                "Restored shutdown changed the desired disk state or left active modules.");
            std::cout << "PASS: registration, restore-only, denied entitlement, service/app activation, events, "
                         "single UI/app slot, native app/UI windows, disk state roundtrip, shutdown and restored reboot.\n";
            return 0;
        }
        const int result = presentation.run();
        controller->shutdown();
        return result;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
