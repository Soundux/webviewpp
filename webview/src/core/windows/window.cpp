#if defined(_WIN32)
#include <WebView2.h>
#include <core/windows/window.hpp>
#include <cstdlib>
#include <stdexcept>
#include <thread>

Webview::Window::Window(std::string identifier, std::size_t width, std::size_t height)
    : BaseWindow(std::move(identifier), width, height), instance(GetModuleHandle(nullptr))
{
    if (!instance)
    {
        throw std::runtime_error("GetModuleHandle returned nullptr");
    }

    WNDCLASSEX wndClass;
    wndClass.style = 0;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 0;
    wndClass.hInstance = instance;
    wndClass.lpfnWndProc = WndProc;
    wndClass.lpszMenuName = nullptr;
    wndClass.cbSize = sizeof(WNDCLASSEX);
    wndClass.lpszClassName = this->identifier.c_str();
    wndClass.hCursor = LoadCursor(nullptr, IDC_ARROW);     // NOLINT
    wndClass.hIcon = LoadIcon(nullptr, IDI_APPLICATION);   // NOLINT
    wndClass.hIconSm = LoadIcon(nullptr, IDI_APPLICATION); // NOLINT
    wndClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);

    if (!RegisterClassEx(&wndClass))
    {
        throw std::runtime_error("Failed to register class");
    }

    hwnd = CreateWindow(this->identifier.c_str(), nullptr, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width,
                        height, nullptr, nullptr, instance, nullptr);
    if (!hwnd)
    {
        throw std::runtime_error("Failed to create window");
    }

    RECT rect;
    GetWindowRect(hwnd, &rect);
    auto x = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
    auto y = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;
    SetWindowPos(hwnd, nullptr, x, y, width, height, 0);

    SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE); // NOLINT
    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);
    SetFocus(hwnd);

    char *buffer = nullptr;
    std::size_t bufferLength = 0;
    _dupenv_s(&buffer, &bufferLength, "LOCALAPPDATA");

    std::string appdata(buffer, bufferLength);
    appdata += "\\MicrosoftEdge";

    if (FAILED(createEnvironment(appdata)))
    {
        throw std::runtime_error("Failed to create environment");
    }
}

LRESULT CALLBACK Webview::Window::Window::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    auto *webview = reinterpret_cast<Window *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    if (webview)
    {
        switch (msg)
        {
        case WM_SIZE:
            webview->onResize(LOWORD(lParam), HIWORD(lParam));
            break;
        case WM_CLOSE:
            if (webview)
            {
                if (webview->onClose())
                {
                    return 0;
                }

                DestroyWindow(hwnd);
            }
            break;
        case WM_QUIT:
            webview->onClose();
            DestroyWindow(hwnd);
            break;
        case WM_CALL: {
            auto *func = reinterpret_cast<std::function<void()> *>(wParam);
            (*func)();
            delete func;
        }
        break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }
    }
    else
    {
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return 0;
}

HRESULT Webview::Window::Window::createEnvironment(const std::string &appdata)
{
    return CreateCoreWebView2EnvironmentWithOptions(
        nullptr, widen(appdata).c_str(), nullptr,
        Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [this](auto res, ICoreWebView2Environment *env) -> HRESULT {
                if (FAILED(res))
                {
                    return res;
                }

                return env->CreateCoreWebView2Controller(
                    hwnd, Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                              [&](auto result, ICoreWebView2Controller *controller) {
                                  if (FAILED(result))
                                  {
                                      return result;
                                  }
                                  return onControllerCreated(controller);
                              })
                              .Get());
            })
            .Get());
}

HRESULT Webview::Window::Window::onControllerCreated(ICoreWebView2Controller *controller)
{
    if (!controller)
    {
        throw std::runtime_error("Failed to create controller");
    }

    webViewController = controller;
    webViewController->get_CoreWebView2(&webViewWindow);

    EventRegistrationToken navigationStarting;
    webViewWindow->add_NavigationStarting(
        Microsoft::WRL::Callback<ICoreWebView2NavigationStartingEventHandler>([this](auto *sender, auto *args) {
            return onNavigationStarted(sender, args);
        }).Get(),
        &navigationStarting);

    webViewWindow->AddWebResourceRequestedFilter(L"*", COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL);
    EventRegistrationToken webResourceRequested;
    webViewWindow->add_WebResourceRequested(
        Microsoft::WRL::Callback<ICoreWebView2WebResourceRequestedEventHandler>([=](auto *sender, auto *args) {
            return onWebResourceRequested(sender, args);
        }).Get(),
        &webResourceRequested);

    EventRegistrationToken messageReceived;
    webViewWindow->add_WebMessageReceived(
        Microsoft::WRL::Callback<ICoreWebView2WebMessageReceivedEventHandler>([=](auto *sender, auto *args) {
            return onMessageReceived(sender, args);
        }).Get(),
        &messageReceived);

    injectCode("window.external.invoke=arg=>window.chrome.webview.postMessage(arg);");
    injectCode(setupRpc);

    for (const auto &func : runOnControllerCreated)
    {
        func();
    }
    runOnControllerCreated.clear();

    return S_OK;
}

HRESULT Webview::Window::Window::onMessageReceived([[maybe_unused]] ICoreWebView2 *sender,
                                                   ICoreWebView2WebMessageReceivedEventArgs *args)
{
    LPWSTR raw = nullptr;
    args->TryGetWebMessageAsString(&raw);

    auto message = narrow(raw);
    BaseWindow::handleRawCallRequest(message);

    CoTaskMemFree(raw);
    return S_OK;
}

HRESULT Webview::Window::Window::onNavigationStarted([[maybe_unused]] ICoreWebView2 *sender,
                                                     ICoreWebView2NavigationStartingEventArgs *args)
{
    LPWSTR uri = nullptr;
    args->get_Uri(&uri);

    BaseWindow::onNavigate(narrow(uri));
    return S_OK;
}

HRESULT Webview::Window::Window::onWebResourceRequested([[maybe_unused]] ICoreWebView2 *sender,
                                                        ICoreWebView2WebResourceRequestedEventArgs * /*args*/)
{
    // TODO(implement)
    return S_OK;
}

void Webview::Window::Window::hide()
{
    BaseWindow::hide();
    ShowWindow(hwnd, SW_HIDE);
}

void Webview::Window::Window::show()
{
    BaseWindow::show();
    ShowWindow(hwnd, SW_SHOW);
    SetFocus(hwnd);
}

void Webview::Window::Window::run()
{
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void Webview::Window::Window::exit()
{
    PostMessage(hwnd, WM_QUIT, 0, 0);
}

void Webview::Window::setUrl(std::string newUrl)
{
    if (!webViewController)
    {
        runOnControllerCreated.emplace_back([newUrl = std::move(newUrl), this] { setUrl(newUrl); });
        return;
    }

    BaseWindow::setUrl(newUrl);
    webViewWindow->Navigate(widen(url).c_str());
}

void Webview::Window::setTitle(std::string newTitle)
{
    BaseWindow::setTitle(newTitle);
    SetWindowText(hwnd, title.c_str());
}

void Webview::Window::setSize(std::size_t newWidth, std::size_t newHeight)
{
    BaseWindow::setSize(newWidth, newHeight);
    SetWindowPos(hwnd, nullptr, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER | SWP_NOOWNERZORDER);
}

void Webview::Window::enableDevTools(bool state)
{
    if (!webViewController)
    {
        runOnControllerCreated.emplace_back([=] { enableDevTools(state); });
        return;
    }

    wil::com_ptr<ICoreWebView2Settings> settings;

    webViewWindow->get_Settings(&settings);
    settings->put_AreDevToolsEnabled(state);
}

void Webview::Window::enableContextMenu(bool state)
{
    if (!webViewController)
    {
        runOnControllerCreated.emplace_back([=] { enableContextMenu(state); });
        return;
    }

    wil::com_ptr<ICoreWebView2Settings> settings;

    webViewWindow->get_Settings(&settings);
    settings->put_AreDefaultContextMenusEnabled(state);
}

void Webview::Window::dispatchMessage(std::function<void()> func)
{
    auto *funcPtr = new std::function<void()>(std::move(func));
    PostMessage(hwnd, WM_CALL, reinterpret_cast<ULONG_PTR>(funcPtr), 0);
}

void Webview::Window::runCode(const std::string &code)
{
    if (!webViewController)
    {
        runOnControllerCreated.emplace_back([=] { runCode(code); });
        return;
    }
    dispatchMessage([this, code] { webViewWindow->ExecuteScript(widen(formatCode(code)).c_str(), nullptr); });
}

void Webview::Window::injectCode(const std::string &code)
{
    if (!webViewController)
    {
        runOnControllerCreated.emplace_back([=] { injectCode(code); });
        return;
    }
    webViewWindow->AddScriptToExecuteOnDocumentCreated(widen(formatCode(code)).c_str(), nullptr);
}

void Webview::Window::onResize(std::size_t width, std::size_t height)
{
    BaseWindow::onResize(width, height);

    if (!webViewController)
    {
        runOnControllerCreated.emplace_back([=] { onResize(width, height); });
        return;
    }

    RECT rc;
    GetClientRect(hwnd, &rc);
    webViewController->put_Bounds(rc);
}

std::wstring Webview::Window::Window::widen(const std::string &str)
{
    auto wsz = MultiByteToWideChar(65001, 0, str.c_str(), -1, nullptr, 0);
    if (!wsz)
    {
        return std::wstring();
    }

    std::wstring out(wsz, 0);
    MultiByteToWideChar(65001, 0, str.c_str(), -1, &out[0], wsz);
    out.resize(wsz - 1);
    return out;
}
std::string Webview::Window::Window::narrow(const std::wstring &wstr)
{
    auto sz = WideCharToMultiByte(65001, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (!sz)
    {
        return std::string();
    }

    std::string out(sz, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &out[0], sz, nullptr, nullptr);
    return out;
}

#endif