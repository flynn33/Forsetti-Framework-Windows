// Forsetti Framework for Windows
// Copyright (c) 2026 James Daley. All Rights Reserved.
// Proprietary and Confidential. Patent Pending.

#include "ForsettiPlatform/WindowsViewFactoryRegistry.h"

#include <algorithm>
#include <utility>

namespace Forsetti {

WindowsViewFactoryRegistryException::WindowsViewFactoryRegistryException(
    WindowsViewFactoryRegistryError error,
    std::string message)
    : std::runtime_error(std::move(message))
    , error_(error)
{
}

WindowsViewFactoryRegistryError WindowsViewFactoryRegistryException::error() const noexcept
{
    return error_;
}

void ForsettiViewFactoryRegistry::registerFactory(
    const std::string& moduleID,
    const std::string& viewID,
    const std::vector<std::string>& manifestDeclaredViewIDs,
    std::shared_ptr<IWindowsViewFactory> factory)
{
    if (std::find(manifestDeclaredViewIDs.begin(), manifestDeclaredViewIDs.end(), viewID) ==
        manifestDeclaredViewIDs.end()) {
        throw WindowsViewFactoryRegistryException(
            WindowsViewFactoryRegistryError::UndeclaredViewID,
            "View factory ID is not declared by manifest: " + moduleID + ":" + viewID);
    }
    if (!factory) {
        throw std::invalid_argument("View factory must not be null.");
    }

    std::lock_guard lock(mutex_);
    const RegistryKey key{moduleID, viewID};
    if (factories_.contains(key)) {
        throw WindowsViewFactoryRegistryException(
            WindowsViewFactoryRegistryError::DuplicateFactory,
            "View factory is already registered: " + moduleID + ":" + viewID);
    }
    factories_[key] = std::move(factory);
}

std::shared_ptr<IWindowsViewFactory> ForsettiViewFactoryRegistry::resolveFactory(
    const std::string& moduleID,
    const std::string& viewID) const
{
    std::lock_guard lock(mutex_);
    const RegistryKey key{moduleID, viewID};
    const auto it = factories_.find(key);
    if (it == factories_.end()) {
        throw WindowsViewFactoryRegistryException(
            WindowsViewFactoryRegistryError::FactoryNotFound,
            "View factory is not registered: " + moduleID + ":" + viewID);
    }
    return it->second;
}

std::shared_ptr<void> ForsettiViewFactoryRegistry::createView(
    const std::string& moduleID,
    const std::string& viewID,
    IWindowsDispatcher& dispatcher) const
{
    auto factory = resolveFactory(moduleID, viewID);
    std::shared_ptr<void> view;
    dispatcher.dispatch([&]() {
        view = factory->createView(WindowsViewCreationContext{
            .moduleID = moduleID,
            .viewID = viewID
        });
    });

    if (!view) {
        throw WindowsViewFactoryRegistryException(
            WindowsViewFactoryRegistryError::FactoryNotFound,
            "View factory returned an empty view: " + moduleID + ":" + viewID);
    }
    return view;
}

void ImmediateWindowsDispatcher::dispatch(std::function<void()> operation)
{
    operation();
}

} // namespace Forsetti
