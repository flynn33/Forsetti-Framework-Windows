// Forsetti Framework for Windows
// Copyright (c) 2026 James Daley. All Rights Reserved.
// Proprietary and Confidential. Patent Pending.

#include "ForsettiCore/ForsettiContext.h"
#include <utility>

namespace Forsetti {

namespace {

bool containsCapability(const std::set<Capability>& capabilities, Capability capability)
{
    return capabilities.find(capability) != capabilities.end();
}

std::map<std::string, std::string> sanitizedModulePayload(
    std::map<std::string, std::string> payload)
{
    payload.erase("sourceModuleID");
    payload.erase("targetModuleID");
    payload.erase("capability");
    payload.erase("role");
    payload.erase("forsetti.internal");
    return payload;
}

} // namespace

// ---------------------------------------------------------------------------
// DefaultModuleCommunicationGuard
// ---------------------------------------------------------------------------
void DefaultModuleCommunicationGuard::validate(const std::string& sourceModuleID,
                                                const std::string& targetModuleID,
                                                const std::string& eventType) {
    if (sourceModuleID.empty() || targetModuleID.empty()) {
        throw ForsettiContextException(ForsettiContextError::InvalidModuleID);
    }
    if (sourceModuleID == targetModuleID) {
        throw ForsettiContextException(ForsettiContextError::SelfMessageNotAllowed);
    }
    if (eventType.starts_with("forsetti.internal.")) {
        throw ForsettiContextException(ForsettiContextError::ReservedNamespace);
    }
}

// ---------------------------------------------------------------------------
// ForsettiContext — constructor
// ---------------------------------------------------------------------------
ForsettiContext::ForsettiContext(std::shared_ptr<IServiceProvider> services,
                                 std::shared_ptr<IForsettiEventBus> eventBus,
                                 std::shared_ptr<IForsettiLogger> logger,
                                 std::shared_ptr<IOverlayRouter> router,
                                 std::shared_ptr<IModuleCommunicationGuard> guard)
    : services_(std::move(services))
    , eventBus_(std::move(eventBus))
    , logger_(std::move(logger))
    , router_(std::move(router))
    , guard_(std::move(guard)) {}

// ---------------------------------------------------------------------------
// Accessors
// ---------------------------------------------------------------------------
std::shared_ptr<IServiceProvider> ForsettiContext::services() const {
    return services_;
}

std::shared_ptr<IForsettiEventBus> ForsettiContext::eventBus() const {
    return eventBus_;
}

std::shared_ptr<IForsettiLogger> ForsettiContext::logger() const {
    return logger_;
}

std::shared_ptr<IOverlayRouter> ForsettiContext::router() const {
    return router_;
}

const std::string& ForsettiContext::moduleID() const noexcept {
    static const std::string emptyModuleID;
    return moduleID_.has_value() ? moduleID_.value() : emptyModuleID;
}

const std::optional<std::string>& ForsettiContext::scopedModuleID() const noexcept {
    return moduleID_;
}

const std::set<Capability>& ForsettiContext::grantedCapabilities() const noexcept {
    return grantedCapabilities_;
}

std::shared_ptr<ForsettiContext> ForsettiContext::scopedToModule(
    const std::string& moduleID,
    const std::vector<Capability>& grantedCapabilities) const {
    auto grantedSet = std::set<Capability>(
        grantedCapabilities.begin(),
        grantedCapabilities.end());

    auto scopedServices = std::make_shared<CapabilityScopedServiceProvider>(
        services_,
        moduleID,
        grantedSet,
        logger_);

    auto scopedContext = std::make_shared<ForsettiContext>(
        scopedServices,
        eventBus_,
        logger_,
        router_,
        guard_);
    scopedContext->moduleID_ = moduleID;
    scopedContext->grantedCapabilities_ = std::move(grantedSet);
    return scopedContext;
}

// ---------------------------------------------------------------------------
// publishFrameworkEvent — direct publish, no guard
// ---------------------------------------------------------------------------
void ForsettiContext::publishFrameworkEvent(const ForsettiEvent& event) {
    ForsettiEvent frameworkEvent = event;
    frameworkEvent.sourceModuleID = std::nullopt;
    eventBus_->publish(frameworkEvent);
}

void ForsettiContext::publishEvent(
    const std::string& eventType,
    const std::map<std::string, std::string>& payload)
{
    if (!moduleID_.has_value()) {
        throw ForsettiContextException(ForsettiContextError::UnscopedModuleContext);
    }
    if (eventType.starts_with("forsetti.internal.")) {
        throw ForsettiContextException(ForsettiContextError::ReservedNamespace);
    }
    if (!containsCapability(grantedCapabilities_, Capability::EventPublishing)) {
        throw std::runtime_error("Module event publication requires event_publishing capability.");
    }

    ForsettiEvent event;
    event.type = eventType;
    event.sourceModuleID = moduleID_.value();
    event.payload = sanitizedModulePayload(payload);
    eventBus_->publish(event);
}

// ---------------------------------------------------------------------------
// sendModuleMessage — validated module-to-module messaging
// ---------------------------------------------------------------------------
void ForsettiContext::sendModuleMessage(const std::string& targetModuleID,
                                        const std::string& eventType,
                                        const std::map<std::string, std::string>& payload) {
    if (!moduleID_.has_value()) {
        throw ForsettiContextException(ForsettiContextError::UnscopedModuleContext);
    }

    const auto& sourceModuleID = moduleID_.value();

    // Validate via the communication guard
    guard_->validate(sourceModuleID, targetModuleID, eventType);

    // Build the enriched event
    ForsettiEvent event;
    event.type = eventType;
    event.sourceModuleID = sourceModuleID;

    // Start with the caller-supplied payload, then inject framework-owned fields.
    event.payload = sanitizedModulePayload(payload);
    event.payload["targetModuleID"] = targetModuleID;

    eventBus_->publish(event);
}

void ForsettiContext::sendMessage(const std::string& targetModuleID,
                                  const std::string& eventType,
                                  const std::map<std::string, std::string>& payload)
{
    sendModuleMessage(targetModuleID, eventType, payload);
}

// ---------------------------------------------------------------------------
// subscribeToModuleMessages — filtered subscription by targetModuleID
// ---------------------------------------------------------------------------
SubscriptionToken ForsettiContext::subscribeToModuleMessages(
    const std::string& targetModuleID,
    const std::string& eventType,
    std::function<void(const ForsettiEvent&)> handler) {

    // Wrap the handler to filter by targetModuleID in the event payload
    auto filteredHandler = [targetModuleID, handler = std::move(handler)](const ForsettiEvent& event) {
        auto it = event.payload.find("targetModuleID");
        if (it != event.payload.end() && it->second == targetModuleID) {
            handler(event);
        }
    };

    auto id = eventBus_->subscribe(eventType, std::move(filteredHandler));
    return SubscriptionToken(eventBus_.get(), id);
}

SubscriptionToken ForsettiContext::subscribeToMessages(
    const std::string& eventType,
    std::function<void(const ForsettiEvent&)> handler)
{
    if (!moduleID_.has_value()) {
        throw ForsettiContextException(ForsettiContextError::UnscopedModuleContext);
    }
    if (eventType.starts_with("forsetti.internal.")) {
        throw ForsettiContextException(ForsettiContextError::ReservedNamespace);
    }
    return subscribeToModuleMessages(moduleID_.value(), eventType, std::move(handler));
}

// ---------------------------------------------------------------------------
// subscribeToFrameworkEvents — direct subscription, no filtering
// ---------------------------------------------------------------------------
SubscriptionToken ForsettiContext::subscribeToFrameworkEvents(
    const std::string& eventType,
    std::function<void(const ForsettiEvent&)> handler) {
    auto id = eventBus_->subscribe(eventType, std::move(handler));
    return SubscriptionToken(eventBus_.get(), id);
}

} // namespace Forsetti
