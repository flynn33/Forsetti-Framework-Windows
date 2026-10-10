// Forsetti Framework for Windows
// Copyright (c) 2026 James Daley.
// SPDX-License-Identifier: Apache-2.0

#include "NativePresentation.h"
#include <algorithm>
#include <stdexcept>
#include <utility>

namespace {
std::wstring wide(const std::string& utf8)
{
    const int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(),
        static_cast<int>(utf8.size()), nullptr, 0);
    if (length <= 0) { throw std::runtime_error("Invalid UTF-8 presentation text."); }
    std::wstring result(static_cast<std::size_t>(length), L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(), static_cast<int>(utf8.size()),
        result.data(), length);
    return result;
}
struct NativeView final {
    HWND window = nullptr;
    ~NativeView() { if (window && IsWindow(window)) { DestroyWindow(window); } }
};
class TextViewFactory final : public Forsetti::IWindowsViewFactory {
public:
    explicit TextViewFactory(HWND parent) : parent_(parent) {}
    std::shared_ptr<void> createView(const Forsetti::WindowsViewCreationContext& context) override
    {
        auto view = std::make_shared<NativeView>();
        const auto text = wide(context.moduleID + "\n" + context.viewID);
        view->window = CreateWindowExW(0, L"STATIC", text.c_str(), WS_CHILD | WS_VISIBLE | SS_CENTER,
            20, 110, 640, 200, parent_, nullptr, GetModuleHandleW(nullptr), nullptr);
        if (!view->window) { throw std::runtime_error("Cannot create native module view."); }
        return view;
    }
private:
    HWND parent_;
};
}
NativePresentation::NativePresentation(std::shared_ptr<Forsetti::IForsettiHostController> controller,
    std::shared_ptr<Forsetti::IHostSurfaceStateSource> surface,
    const std::vector<Forsetti::ModuleManifest>& manifests)
    : controller_(std::move(controller)), surface_(std::move(surface))
{
    WNDCLASSW windowClass{};
    windowClass.lpfnWndProc = windowProcedure;
    windowClass.hInstance = GetModuleHandleW(nullptr);
    windowClass.lpszClassName = L"TemplateHost.Window";
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    if (!RegisterClassW(&windowClass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        throw std::runtime_error("Cannot register application window class.");
    }
    window_ = CreateWindowExW(0, windowClass.lpszClassName, L"TemplateHost",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 700, 380,
        nullptr, nullptr, windowClass.hInstance, this);
    if (!window_) { throw std::runtime_error("Cannot create application window."); }
    status_ = CreateWindowExW(0, L"STATIC", L"Select a module to activate through Forsetti.",
        WS_CHILD | WS_VISIBLE, 20, 70, 640, 30, window_, nullptr, windowClass.hInstance, nullptr);
    if (!status_) { throw std::runtime_error("Cannot create application status control."); }
    int column = 0;
    for (const auto& manifest : manifests) {
        moduleIDs_.push_back(manifest.moduleID);
        const auto title = wide(manifest.displayName);
        if (!CreateWindowExW(0, L"BUTTON", title.c_str(), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            20 + 215 * column, 20, 205, 32, window_,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(100 + column)), windowClass.hInstance, nullptr)) {
            throw std::runtime_error("Cannot create module activation control.");
        }
        if (manifest.runtimeRequirements.ui) {
            for (const auto& viewID : manifest.runtimeRequirements.ui->viewIDs) {
                factories_.registerFactory(manifest.moduleID, viewID,
                    manifest.runtimeRequirements.ui->viewIDs, std::make_shared<TextViewFactory>(window_));
            }
        }
        ++column;
    }
    ShowWindow(window_, SW_SHOWNOACTIVATE);
    render();
    UpdateWindow(window_);
}
NativePresentation::~NativePresentation()
{
    presentedView_.reset();
    if (window_ && IsWindow(window_)) { DestroyWindow(window_); }
}
void NativePresentation::render()
{
    presentedView_.reset();
    presentedViewID_.clear();
    const auto host = controller_->snapshot();
    const auto state = surface_->snapshot();
    if (!host.activeUIModuleID) {
        SetWindowTextW(status_, L"Ready. No UI/app module is active.");
        return;
    }
    auto content = state.viewInjectionsBySlot.find("content");
    if (content == state.viewInjectionsBySlot.end() || content->second.empty()) {
        throw std::runtime_error("Active module has no validated content view.");
    }
    const auto& injection = content->second.front();
    Forsetti::ImmediateWindowsDispatcher dispatcher;
    presentedView_ = factories_.createView(*host.activeUIModuleID, injection.viewID, dispatcher);
    presentedViewID_ = injection.viewID;
    const auto text = wide("Active surface: " + *host.activeUIModuleID);
    SetWindowTextW(status_, text.c_str());
    UpdateWindow(window_);
}
bool NativePresentation::hasPresentedView(const std::string& expectedViewID) const
{
    if (!presentedView_ || presentedViewID_ != expectedViewID) { return false; }
    const auto view = std::static_pointer_cast<NativeView>(presentedView_);
    if (!IsWindow(window_) || !IsWindowVisible(window_) || !IsWindow(view->window) ||
        !IsWindowVisible(view->window)) { return false; }
    wchar_t text[512]{};
    GetWindowTextW(view->window, text, 512);
    return std::wstring(text).find(wide(expectedViewID)) != std::wstring::npos;
}
void NativePresentation::command(unsigned int id)
{
    if (id < 100 || id - 100 >= moduleIDs_.size()) { return; }
    try {
        const auto& moduleID = moduleIDs_[id - 100];
        const auto state = controller_->snapshot();
        if (std::find(state.activeModuleIDs.begin(), state.activeModuleIDs.end(), moduleID) !=
            state.activeModuleIDs.end()) {
            controller_->deactivateModule(moduleID);
        } else {
            controller_->activateModule(moduleID);
        }
        render();
    } catch (const std::exception& error) {
        const auto text = wide(error.what());
        SetWindowTextW(status_, text.c_str());
    }
}
LRESULT CALLBACK NativePresentation::windowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    auto* owner = reinterpret_cast<NativePresentation*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        owner = static_cast<NativePresentation*>(reinterpret_cast<CREATESTRUCTW*>(lParam)->lpCreateParams);
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(owner));
    }
    if (owner && message == WM_COMMAND) { owner->command(LOWORD(wParam)); return 0; }
    if (message == WM_CLOSE) { DestroyWindow(window); return 0; }
    if (message == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProcW(window, message, wParam, lParam);
}
int NativePresentation::run()
{
    MSG message{};
    int status;
    while ((status = static_cast<int>(GetMessageW(&message, nullptr, 0, 0))) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    if (status < 0) { throw std::runtime_error("Application message loop failed."); }
    return static_cast<int>(message.wParam);
}
