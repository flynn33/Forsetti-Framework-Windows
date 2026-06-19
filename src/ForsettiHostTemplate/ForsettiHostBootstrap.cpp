// Forsetti Framework for Windows
// Copyright (c) 2026 James Daley. All Rights Reserved.
// Proprietary and Confidential. Patent Pending.

#include "ForsettiHostTemplate/ForsettiHostBootstrap.h"
#include "ForsettiHostTemplate/ForsettiHostOverlayRouter.h"
#include "ForsettiCore/ForsettiVersion.h"

#include <stdexcept>
#include <utility>

namespace Forsetti {

std::shared_ptr<IForsettiHostController> ForsettiHostBootstrap::makeController(
    ForsettiHostBootstrapConfiguration configuration) const
{
    if (configuration.manifestDirectory.empty()) {
        throw std::invalid_argument("ForsettiHostBootstrap requires a manifest directory.");
    }
    if (!configuration.activationStore) {
        throw std::invalid_argument("ForsettiHostBootstrap requires an activation store.");
    }

    auto services = configuration.services
        ? configuration.services
        : std::make_shared<ServiceContainer>();
    auto eventBus = configuration.eventBus
        ? configuration.eventBus
        : std::make_shared<InMemoryEventBus>();
    auto logger = configuration.logger
        ? configuration.logger
        : std::make_shared<ConsoleLogger>();
    auto overlayRouter = configuration.overlayRouter
        ? configuration.overlayRouter
        : std::make_shared<ForsettiHostOverlayRouter>();
    auto communicationGuard = configuration.communicationGuard
        ? configuration.communicationGuard
        : std::make_shared<DefaultModuleCommunicationGuard>();
    auto surfaceManager = configuration.surfaceManager
        ? configuration.surfaceManager
        : std::make_shared<UISurfaceManager>();
    auto entitlementProvider = configuration.entitlementProvider
        ? configuration.entitlementProvider
        : std::make_shared<AllowAllEntitlementProvider>();
    auto capabilityPolicy = configuration.capabilityPolicy
        ? configuration.capabilityPolicy
        : std::make_shared<AllowAllCapabilityPolicy>();
    auto registrationService = configuration.registrationService
        ? configuration.registrationService
        : ModuleRegistrationService::makeInMemory();
    auto requirementValidator = configuration.requirementValidator
        ? configuration.requirementValidator
        : std::make_shared<ModuleRequirementValidator>();

    auto context = std::make_shared<ForsettiContext>(
        services,
        eventBus,
        logger,
        overlayRouter,
        communicationGuard);
    auto checker = std::make_shared<CompatibilityChecker>(
        ForsettiVersion::current,
        capabilityPolicy);

    auto manager = std::make_unique<ModuleManager>(
        std::move(configuration.moduleRegistry),
        checker,
        entitlementProvider,
        configuration.activationStore,
        surfaceManager,
        context,
        registrationService,
        requirementValidator);
    auto runtime = std::make_shared<ForsettiRuntime>(
        std::move(manager),
        entitlementProvider,
        eventBus,
        std::move(configuration.manifestDirectory));

    return std::make_shared<ForsettiHostController>(runtime);
}

} // namespace Forsetti
