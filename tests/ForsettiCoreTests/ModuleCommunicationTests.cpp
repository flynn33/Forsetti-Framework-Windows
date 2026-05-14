// Forsetti Framework for Windows
// Copyright (c) 2026 James Daley. All Rights Reserved.
// Proprietary and Confidential. Patent Pending.

#include "CppUnitTest.h"
#include "TestHelpers.h"
#include "ForsettiCore/ForsettiContext.h"
#include "ForsettiCore/ForsettiEventBus.h"
#include "ForsettiCore/ForsettiServiceContainer.h"
#include "ForsettiCore/ForsettiLogger.h"
#include "ForsettiCore/ForsettiServices.h"
#include <string>
#include <vector>
#include <memory>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Forsetti;
using namespace Forsetti::Tests;

namespace {
    class TestStorageService final : public IStorageService {
    public:
        void set(const std::string& key, const std::string& value) override {
            store_[key] = value;
        }

        std::optional<std::string> get(const std::string& key) override {
            auto it = store_.find(key);
            if (it == store_.end()) {
                return std::nullopt;
            }
            return it->second;
        }

        void remove(const std::string& key) override {
            store_.erase(key);
        }

    private:
        std::map<std::string, std::string> store_;
    };

    class TestSecureStorageService final : public ISecureStorageService {
    public:
        void set(const std::string& key, const std::vector<uint8_t>& data) override {
            store_[key] = data;
        }

        std::optional<std::vector<uint8_t>> get(const std::string& key) override {
            auto it = store_.find(key);
            if (it == store_.end()) {
                return std::nullopt;
            }
            return it->second;
        }

        void remove(const std::string& key) override {
            store_.erase(key);
        }

    private:
        std::map<std::string, std::vector<uint8_t>> store_;
    };

    std::shared_ptr<ForsettiContext> makeTestContext(
        std::shared_ptr<IForsettiEventBus> bus = nullptr,
        std::shared_ptr<IModuleCommunicationGuard> guard = nullptr,
        std::shared_ptr<ServiceContainer> services = nullptr,
        std::shared_ptr<IForsettiLogger> logger = nullptr)
    {
        if (!services) services = std::make_shared<ServiceContainer>();
        if (!bus) bus = std::make_shared<InMemoryEventBus>();
        if (!logger) logger = std::make_shared<ConsoleLogger>();
        auto router = std::make_shared<NoopOverlayRouter>();
        if (!guard) guard = std::make_shared<DefaultModuleCommunicationGuard>();

        return std::make_shared<ForsettiContext>(services, bus, logger, router, guard);
    }

    std::shared_ptr<ForsettiContext> makeScopedTestContext(
        const std::string& moduleID,
        std::vector<Capability> capabilities = {},
        std::shared_ptr<IForsettiEventBus> bus = nullptr,
        std::shared_ptr<ServiceContainer> services = nullptr,
        std::shared_ptr<IForsettiLogger> logger = nullptr)
    {
        return makeTestContext(bus, nullptr, services, logger)->scopedToModule(moduleID, capabilities);
    }
}

TEST_CLASS(EventBusTests)
{
public:

    TEST_METHOD(Publish_SubscriberReceivesEvent)
    {
        InMemoryEventBus bus;
        std::string received;

        bus.subscribe("test.event", [&received](const ForsettiEvent& e) {
            received = e.type;
        });

        ForsettiEvent event;
        event.type = "test.event";
        bus.publish(event);

        Assert::AreEqual(std::string("test.event"), received);
    }

    TEST_METHOD(Publish_OnlyMatchingSubscribersReceive)
    {
        InMemoryEventBus bus;
        int count = 0;

        bus.subscribe("target.event", [&count](const ForsettiEvent&) { count++; });
        bus.subscribe("other.event", [&count](const ForsettiEvent&) { count += 100; });

        ForsettiEvent event;
        event.type = "target.event";
        bus.publish(event);

        Assert::AreEqual(1, count);
    }

    TEST_METHOD(Publish_MultipleSubscribersReceive)
    {
        InMemoryEventBus bus;
        int count = 0;

        bus.subscribe("test.event", [&count](const ForsettiEvent&) { count++; });
        bus.subscribe("test.event", [&count](const ForsettiEvent&) { count++; });

        ForsettiEvent event;
        event.type = "test.event";
        bus.publish(event);

        Assert::AreEqual(2, count);
    }

    TEST_METHOD(Unsubscribe_StopsReceiving)
    {
        InMemoryEventBus bus;
        int count = 0;

        auto tokenID = bus.subscribe("test.event", [&count](const ForsettiEvent&) { count++; });

        ForsettiEvent event;
        event.type = "test.event";
        bus.publish(event);
        Assert::AreEqual(1, count);

        bus.unsubscribe(tokenID);
        bus.publish(event);
        Assert::AreEqual(1, count);  // Should not have incremented
    }

    TEST_METHOD(Publish_PayloadPreserved)
    {
        InMemoryEventBus bus;
        std::string value;

        bus.subscribe("data.event", [&value](const ForsettiEvent& e) {
            auto it = e.payload.find("key");
            if (it != e.payload.end()) value = it->second;
        });

        ForsettiEvent event;
        event.type = "data.event";
        event.payload["key"] = "hello";
        bus.publish(event);

        Assert::AreEqual(std::string("hello"), value);
    }

    TEST_METHOD(SubscriptionToken_CancelStopsReceiving)
    {
        InMemoryEventBus bus;
        int count = 0;

        auto id = bus.subscribe("test.event", [&count](const ForsettiEvent&) { count++; });
        auto token = bus.makeToken(id);

        ForsettiEvent event;
        event.type = "test.event";
        bus.publish(event);
        Assert::AreEqual(1, count);

        token.cancel();
        bus.publish(event);
        Assert::AreEqual(1, count);
    }

    TEST_METHOD(SubscriptionToken_DestructorCancels)
    {
        InMemoryEventBus bus;
        int count = 0;

        {
            auto id = bus.subscribe("test.event", [&count](const ForsettiEvent&) { count++; });
            auto token = bus.makeToken(id);
            // token goes out of scope here
        }

        ForsettiEvent event;
        event.type = "test.event";
        bus.publish(event);
        Assert::AreEqual(0, count);
    }
};

TEST_CLASS(CommunicationGuardTests)
{
public:

    TEST_METHOD(ValidMessage_NoException)
    {
        DefaultModuleCommunicationGuard guard;
        guard.validate("module.a", "module.b", "custom.event");
        // No exception = success
        Assert::IsTrue(true);
    }

    TEST_METHOD(EmptySourceID_Throws)
    {
        DefaultModuleCommunicationGuard guard;
        Assert::ExpectException<ForsettiContextException>([&guard]() {
            guard.validate("", "module.b", "custom.event");
        });
    }

    TEST_METHOD(EmptyTargetID_Throws)
    {
        DefaultModuleCommunicationGuard guard;
        Assert::ExpectException<ForsettiContextException>([&guard]() {
            guard.validate("module.a", "", "custom.event");
        });
    }

    TEST_METHOD(BothIDsEmpty_Throws)
    {
        DefaultModuleCommunicationGuard guard;
        Assert::ExpectException<ForsettiContextException>([&guard]() {
            guard.validate("", "", "custom.event");
        });
    }

    TEST_METHOD(SelfMessage_Throws)
    {
        DefaultModuleCommunicationGuard guard;
        Assert::ExpectException<ForsettiContextException>([&guard]() {
            guard.validate("module.a", "module.a", "custom.event");
        });
    }

    TEST_METHOD(ReservedNamespace_Throws)
    {
        DefaultModuleCommunicationGuard guard;
        Assert::ExpectException<ForsettiContextException>([&guard]() {
            guard.validate("module.a", "module.b", "forsetti.internal.shutdown");
        });
    }

    TEST_METHOD(ReservedNamespace_PrefixOnly)
    {
        DefaultModuleCommunicationGuard guard;
        // "forsetti.internal" without trailing dot should NOT throw
        guard.validate("module.a", "module.b", "forsetti.internal");
        Assert::IsTrue(true);
    }

    TEST_METHOD(ErrorCode_InvalidModuleID)
    {
        DefaultModuleCommunicationGuard guard;
        try {
            guard.validate("", "module.b", "event");
            Assert::Fail(L"Expected exception");
        } catch (const ForsettiContextException& e) {
            Assert::IsTrue(e.error() == ForsettiContextError::InvalidModuleID);
        }
    }

    TEST_METHOD(ErrorCode_SelfMessage)
    {
        DefaultModuleCommunicationGuard guard;
        try {
            guard.validate("module.a", "module.a", "event");
            Assert::Fail(L"Expected exception");
        } catch (const ForsettiContextException& e) {
            Assert::IsTrue(e.error() == ForsettiContextError::SelfMessageNotAllowed);
        }
    }

    TEST_METHOD(ErrorCode_ReservedNamespace)
    {
        DefaultModuleCommunicationGuard guard;
        try {
            guard.validate("module.a", "module.b", "forsetti.internal.test");
            Assert::Fail(L"Expected exception");
        } catch (const ForsettiContextException& e) {
            Assert::IsTrue(e.error() == ForsettiContextError::ReservedNamespace);
        }
    }
};

TEST_CLASS(ForsettiContextMessagingTests)
{
public:

    TEST_METHOD(SendModuleMessage_PublishesToEventBus)
    {
        auto bus = std::make_shared<InMemoryEventBus>();
        auto ctx = makeScopedTestContext("source.module", {}, bus);

        std::string receivedType;
        bus->subscribe("custom.msg", [&receivedType](const ForsettiEvent& e) {
            receivedType = e.type;
        });

        ctx->sendModuleMessage("target.module", "custom.msg");
        Assert::AreEqual(std::string("custom.msg"), receivedType);
    }

    TEST_METHOD(SendModuleMessage_InjectsTargetModuleID)
    {
        auto bus = std::make_shared<InMemoryEventBus>();
        auto ctx = makeScopedTestContext("source", {}, bus);

        std::string targetID;
        bus->subscribe("msg.type", [&targetID](const ForsettiEvent& e) {
            auto it = e.payload.find("targetModuleID");
            if (it != e.payload.end()) targetID = it->second;
        });

        ctx->sendModuleMessage("target.mod", "msg.type");
        Assert::AreEqual(std::string("target.mod"), targetID);
    }

    TEST_METHOD(SendModuleMessage_SetsSourceModuleID)
    {
        auto bus = std::make_shared<InMemoryEventBus>();
        auto ctx = makeScopedTestContext("source.mod", {}, bus);

        std::string sourceID;
        bus->subscribe("msg.type", [&sourceID](const ForsettiEvent& e) {
            if (e.sourceModuleID.has_value()) sourceID = e.sourceModuleID.value();
        });

        ctx->sendModuleMessage("target.mod", "msg.type");
        Assert::AreEqual(std::string("source.mod"), sourceID);
    }

    TEST_METHOD(SendModuleMessage_InvalidIDs_Throws)
    {
        auto ctx = makeScopedTestContext("source.mod");
        Assert::ExpectException<ForsettiContextException>([&ctx]() {
            ctx->sendModuleMessage("", "event");
        });
    }

    TEST_METHOD(SendModuleMessage_UnscopedContext_Throws)
    {
        auto ctx = makeTestContext();
        try {
            ctx->sendModuleMessage("target", "event");
            Assert::Fail(L"Expected exception");
        } catch (const ForsettiContextException& e) {
            Assert::IsTrue(e.error() == ForsettiContextError::UnscopedModuleContext);
        }
    }

    TEST_METHOD(SendModuleMessage_CannotSpoofSourceInPayload)
    {
        auto bus = std::make_shared<InMemoryEventBus>();
        auto ctx = makeScopedTestContext("source.mod", {}, bus);

        std::string sourceID;
        bool payloadContainsSource = false;
        bus->subscribe("msg.type", [&sourceID, &payloadContainsSource](const ForsettiEvent& e) {
            if (e.sourceModuleID.has_value()) sourceID = e.sourceModuleID.value();
            payloadContainsSource = e.payload.contains("sourceModuleID");
        });

        ctx->sendModuleMessage(
            "target.mod",
            "msg.type",
            {{"sourceModuleID", "other.mod"}});

        Assert::AreEqual(std::string("source.mod"), sourceID);
        Assert::IsFalse(payloadContainsSource);
    }

    TEST_METHOD(ScopedServices_StorageDeniedWithoutCapability)
    {
        auto services = std::make_shared<ServiceContainer>();
        services->registerService<IStorageService>(std::make_shared<TestStorageService>());
        auto logger = std::make_shared<RecordingLogger>();
        auto ctx = makeScopedTestContext("source.mod", {}, nullptr, services, logger);

        auto storage = ctx->services()->resolve<IStorageService>();
        Assert::IsNull(storage.get());

        Assert::AreEqual(size_t(1), logger->entries.size());
        Assert::IsTrue(logger->entries[0].level == LogLevel::Warning);
        Assert::AreEqual(std::string("source.mod"), logger->entries[0].sourceModuleID);
        Assert::IsTrue(logger->entries[0].message.find("storage") != std::string::npos);
    }

    TEST_METHOD(ScopedServices_StorageAllowedWithCapability)
    {
        auto services = std::make_shared<ServiceContainer>();
        auto expected = std::make_shared<TestStorageService>();
        services->registerService<IStorageService>(expected);
        auto logger = std::make_shared<RecordingLogger>();
        auto ctx = makeScopedTestContext(
            "source.mod",
            {Capability::Storage},
            nullptr,
            services,
            logger);

        auto storage = ctx->services()->resolve<IStorageService>();
        Assert::IsNotNull(storage.get());
        Assert::IsTrue(storage == expected);
        Assert::IsTrue(logger->entries.empty());
    }

    TEST_METHOD(ScopedServices_SecureStorageDeniedWithoutCapability)
    {
        auto services = std::make_shared<ServiceContainer>();
        services->registerService<ISecureStorageService>(std::make_shared<TestSecureStorageService>());
        auto logger = std::make_shared<RecordingLogger>();
        auto ctx = makeScopedTestContext(
            "source.mod",
            {Capability::Storage},
            nullptr,
            services,
            logger);

        auto secureStorage = ctx->services()->resolve<ISecureStorageService>();
        Assert::IsNull(secureStorage.get());

        Assert::AreEqual(size_t(1), logger->entries.size());
        Assert::IsTrue(logger->entries[0].level == LogLevel::Warning);
        Assert::AreEqual(std::string("source.mod"), logger->entries[0].sourceModuleID);
        Assert::IsTrue(logger->entries[0].message.find("secure_storage") != std::string::npos);
    }

    TEST_METHOD(ScopedContext_ExposesModuleIdentityAndCapabilities)
    {
        auto ctx = makeScopedTestContext(
            "source.mod",
            {Capability::Storage, Capability::EventPublishing});

        Assert::IsTrue(ctx->moduleID().has_value());
        Assert::AreEqual(std::string("source.mod"), ctx->moduleID().value());
        Assert::IsTrue(ctx->grantedCapabilities().contains(Capability::Storage));
        Assert::IsTrue(ctx->grantedCapabilities().contains(Capability::EventPublishing));
    }

    TEST_METHOD(PublishFrameworkEvent_BypassesGuard)
    {
        auto bus = std::make_shared<InMemoryEventBus>();
        auto ctx = makeTestContext(bus);

        std::string received;
        bool hasSource = true;
        bus->subscribe("forsetti.internal.boot", [&received, &hasSource](const ForsettiEvent& e) {
            received = e.type;
            hasSource = e.sourceModuleID.has_value();
        });

        // Framework events can use reserved namespace
        ForsettiEvent event;
        event.type = "forsetti.internal.boot";
        event.sourceModuleID = "source.mod";
        ctx->publishFrameworkEvent(event);
        Assert::AreEqual(std::string("forsetti.internal.boot"), received);
        Assert::IsFalse(hasSource);
    }

    TEST_METHOD(ScopedModuleMessage_ReservedNamespaceStillThrows)
    {
        auto ctx = makeScopedTestContext("source.mod");
        Assert::ExpectException<ForsettiContextException>([&ctx]() {
            ctx->sendModuleMessage("target.mod", "forsetti.internal.boot");
        });
    }
};
