// Forsetti Framework for Windows
// Copyright (c) 2026 James Daley. All Rights Reserved.
// Proprietary and Confidential. Patent Pending.

#pragma once

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace Forsetti {

enum class WindowsViewFactoryRegistryError {
    UndeclaredViewID,
    DuplicateFactory,
    FactoryNotFound
};

class WindowsViewFactoryRegistryException final : public std::runtime_error {
public:
    WindowsViewFactoryRegistryException(
        WindowsViewFactoryRegistryError error,
        std::string message);

    [[nodiscard]] WindowsViewFactoryRegistryError error() const noexcept;

private:
    WindowsViewFactoryRegistryError error_;
};

struct WindowsViewCreationContext final {
    std::string moduleID;
    std::string viewID;
};

class IWindowsViewFactory {
public:
    virtual std::shared_ptr<void> createView(const WindowsViewCreationContext& context) = 0;
    virtual ~IWindowsViewFactory() = default;
};

class IWindowsDispatcher {
public:
    virtual void dispatch(std::function<void()> operation) = 0;
    virtual ~IWindowsDispatcher() = default;
};

class IWindowsViewFactoryRegistry {
public:
    virtual void registerFactory(
        const std::string& moduleID,
        const std::string& viewID,
        const std::vector<std::string>& manifestDeclaredViewIDs,
        std::shared_ptr<IWindowsViewFactory> factory) = 0;
    virtual std::shared_ptr<IWindowsViewFactory> resolveFactory(
        const std::string& moduleID,
        const std::string& viewID) const = 0;
    virtual std::shared_ptr<void> createView(
        const std::string& moduleID,
        const std::string& viewID,
        IWindowsDispatcher& dispatcher) const = 0;

    virtual ~IWindowsViewFactoryRegistry() = default;
};

class ForsettiViewFactoryRegistry final : public IWindowsViewFactoryRegistry {
public:
    void registerFactory(
        const std::string& moduleID,
        const std::string& viewID,
        const std::vector<std::string>& manifestDeclaredViewIDs,
        std::shared_ptr<IWindowsViewFactory> factory) override;
    std::shared_ptr<IWindowsViewFactory> resolveFactory(
        const std::string& moduleID,
        const std::string& viewID) const override;
    std::shared_ptr<void> createView(
        const std::string& moduleID,
        const std::string& viewID,
        IWindowsDispatcher& dispatcher) const override;

private:
    using RegistryKey = std::pair<std::string, std::string>;

    mutable std::mutex mutex_;
    std::map<RegistryKey, std::shared_ptr<IWindowsViewFactory>> factories_;
};

class ImmediateWindowsDispatcher final : public IWindowsDispatcher {
public:
    void dispatch(std::function<void()> operation) override;
};

} // namespace Forsetti
