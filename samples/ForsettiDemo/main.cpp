// Forsetti Framework for Windows
// Copyright (c) 2026 James Daley. All Rights Reserved.
// Proprietary and Confidential. Patent Pending.

#include "ExampleAppModule.h"
#include "ExampleServiceModule.h"
#include "ExampleUIModule.h"
#include "ForsettiCore/ActivationStore.h"
#include "ForsettiHostTemplate/ForsettiHostBootstrap.h"

#include <memory>
#include <string>
#include <utility>

namespace {

class DemoActivationStore final : public Forsetti::IActivationStore {
public:
    Forsetti::ActivationState loadState() const override
    {
        return state_;
    }

    void saveState(const Forsetti::ActivationState& state) override
    {
        state_ = state;
    }

private:
    Forsetti::ActivationState state_;
};

} // namespace

int main(int argc, char** argv)
{
    std::string manifestDirectory = "ForsettiManifests";
    if (argc > 1 && argv[1] != nullptr) {
        manifestDirectory = argv[1];
    }

    Forsetti::ModuleRegistry registry;
    Forsetti::registerExampleServiceModule(registry);
    Forsetti::registerExampleUIModule(registry);
    Forsetti::registerExampleAppModule(registry);

    Forsetti::ForsettiHostBootstrapConfiguration configuration{
        .manifestDirectory = std::move(manifestDirectory),
        .moduleRegistry = std::move(registry),
        .activationStore = std::make_shared<DemoActivationStore>()
    };

    const Forsetti::ForsettiHostBootstrap bootstrap;
    auto controller = bootstrap.makeController(std::move(configuration));
    (void)controller->snapshot();
    return 0;
}
