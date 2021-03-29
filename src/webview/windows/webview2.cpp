#if defined(_WIN32)
#include "webview2.hpp"

namespace Soundux
{
    static constexpr auto WM_CALL = WM_USER + 2;

    std::wstring widen(const std::string &s)
    {
        int wsz = MultiByteToWideChar(65001, 0, s.c_str(), -1, nullptr, 0);
        if (!wsz)
            return std::wstring();

        std::wstring out(wsz, 0);
        MultiByteToWideChar(65001, 0, s.c_str(), -1, &out[0], wsz);
        out.resize(wsz - 1);
        return out;
    }
    std::string narrow(const std::wstring &wstr)
    {
        auto count = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.length()), nullptr, 0, nullptr,
                                         nullptr);
        std::string str(count, 0);
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &str[0], count, nullptr, nullptr);
        return str;
    }

    LRESULT CALLBACK WebView2::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        auto *webView = reinterpret_cast<WebView2 *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

        switch (msg)
        {
        case WM_SIZE:
            if (webView && webView->initDone)
            {
                webView->onResize(LOWORD(lParam), HIWORD(lParam));
            }
            break;
        case WM_CLOSE:
            if (webView->shouldHideOnExit)
            {
                webView->hide();
                webView->onClosed();

                return 0;
            }
            else
            {
                webView->onClosed();
                DestroyWindow(hwnd);
                webView->onExit();
            }
            break;
        case WM_QUIT:
            webView->onClosed();
            DestroyWindow(hwnd);
            webView->onExit();

        case WM_CALL: {
            auto *call = reinterpret_cast<ThreadSafeCall *>(wParam);
            call->func();
            delete call;
        }
        break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }

        return 0;
    }
    bool WebView2::setup(int width, int height)
    {
        this->width = width;
        this->height = height;
        instance = GetModuleHandle(nullptr);
        if (instance == nullptr)
        {
            return false;
        }

        WNDCLASSEX wndClass;
        wndClass.style = 0;
        wndClass.cbClsExtra = 0;
        wndClass.cbWndExtra = 0;
        wndClass.hInstance = instance;
        wndClass.lpfnWndProc = WndProc;
        wndClass.lpszMenuName = nullptr;
        wndClass.cbSize = sizeof(WNDCLASSEX);
        wndClass.lpszClassName = "soundux_webview";
        wndClass.hCursor = LoadCursor(nullptr, IDC_ARROW);     // NOLINT
        wndClass.hIcon = LoadIcon(nullptr, IDI_APPLICATION);   // NOLINT
        wndClass.hIconSm = LoadIcon(nullptr, IDI_APPLICATION); // NOLINT
        wndClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);

        if (!RegisterClassEx(&wndClass))
        {
            return false;
        }

        hwnd = CreateWindow("soundux_webview", nullptr, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 640, 480,
                            nullptr, nullptr, instance, nullptr);

        if (!hwnd)
        {
            return false;
        }

        RECT rect;
        GetWindowRect(hwnd, &rect);
        auto x = (GetSystemMetrics(SM_CXSCREEN) - rect.right) / 2;
        auto y = (GetSystemMetrics(SM_CYSCREEN) - rect.bottom) / 2;
        SetWindowPos(hwnd, nullptr, x, y, width, height, SWP_NOMOVE | SWP_NOZORDER | SWP_NOOWNERZORDER);

        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
        SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE); // NOLINT
        ShowWindow(hwnd, SW_SHOWDEFAULT);
        UpdateWindow(hwnd);
        SetFocus(hwnd);

        auto envResult = CreateCoreWebView2Environment(
            Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
                [&](auto res, ICoreWebView2Environment *env) -> HRESULT {
                    auto controllerResult = env->CreateCoreWebView2Controller(
                        hwnd, Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                                  [&](auto _res, ICoreWebView2Controller *controller) -> HRESULT {
                                      if (FAILED(_res))
                                      {
                                          return res;
                                      }

                                      if (controller)
                                      {
                                          webViewController = controller;
                                          webViewController->get_CoreWebView2(&webViewWindow);
                                      }

                                      EventRegistrationToken navigationCompleted;
                                      webViewWindow->add_NavigationCompleted(
                                          Microsoft::WRL::Callback<ICoreWebView2NavigationCompletedEventHandler>(
                                              [this](auto *webview, auto *args) {
                                                  wil::unique_cotaskmem_string uri;
                                                  webview->get_Source(&uri);
                                                  onNavigate(narrow(uri.get()));
                                                  return S_OK;
                                              })
                                              .Get(),
                                          &navigationCompleted);

                                      EventRegistrationToken messageReceived;
                                      webViewWindow->add_WebMessageReceived(
                                          Microsoft::WRL::Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                                              [this]([[maybe_unused]] auto *webview, auto *args) {
                                                  LPWSTR raw = nullptr;
                                                  args->TryGetWebMessageAsString(&raw);

                                                  auto message = narrow(raw);

                                                  if (!callbacks.empty())
                                                  {
                                                      WebView::resolveCallback(message);
                                                  }

                                                  CoTaskMemFree(raw);
                                                  return S_OK;
                                              })
                                              .Get(),
                                          &messageReceived);

                                      initDone = true;
                                      runCode("window.external.invoke=arg=>window.chrome.webview.postMessage(arg);",
                                              true);
                                      runCode(setup_code, true);

                                      onResize(width, height);

                                      for (auto &fn : runOnInitDone)
                                      {
                                          fn();
                                      }
                                      runOnInitDone.clear();

                                      return S_OK;
                                  })
                                  .Get());
                    return controllerResult;
                })
                .Get());

        return !FAILED(envResult);
    }
    void WebView2::run()
    {
        while (!shouldExit)
        {
            if (GetMessage(&msg, nullptr, 0, 0))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            else
            {
                break;
            }
        }
    }
    void WebView2::setTitle(const std::string &title)
    {
        assert(hwnd != nullptr);
        SetWindowText(hwnd, title.c_str());
    }
    void WebView2::setSize(int _width, int _height)
    {
        SetWindowPos(hwnd, nullptr, 0, 0, _width, _height, SWP_NOMOVE | SWP_NOZORDER | SWP_NOOWNERZORDER);
        WebView::onResize(_width, _height);
    }
    void WebView2::onResize(int _width, int _height)
    {
        if (!initDone)
        {
            runOnInitDone.push_back([=] { setSize(_width, _height); });
        }
        else
        {
            RECT rc;
            GetClientRect(hwnd, &rc);
            webViewController->put_Bounds(rc);
        }
    }
    void WebView2::navigate(const std::string &_url)
    {
        if (!initDone)
        {
            runOnInitDone.push_back([=] { navigate(_url); });
        }
        else
        {
            WebView::navigate(_url);
            assert(webViewWindow != nullptr);
            webViewWindow->Navigate(widen(_url).c_str());
        }
    }
    void WebView2::enableDevTools(bool enable)
    {
        if (!initDone)
        {
            runOnInitDone.push_back([=] { enableDevTools(enable); });
        }
        else
        {
            WebView::enableDevTools(enable);
            assert(webViewWindow != nullptr);
            wil::com_ptr<ICoreWebView2Settings> settings;

            webViewWindow->get_Settings(&settings);
            settings->put_AreDevToolsEnabled(enable);
            settings->put_AreDefaultContextMenusEnabled(enable);
        }
    }
    void WebView2::runCode(const std::string &code, bool inject)
    {
        auto formattedCode = std::regex_replace(code, std::regex(R"rgx(\\")rgx"), R"(\\\")");
        formattedCode = std::regex_replace(formattedCode, std::regex(R"rgx(\\n)rgx"), R"(\\n)");
        formattedCode = std::regex_replace(formattedCode, std::regex(R"rgx(\\t)rgx"), R"(\\t)");

        if (inject || !initDone)
        {
            runOnInitDone.push_back(
                [=] { webViewWindow->AddScriptToExecuteOnDocumentCreated(widen(formattedCode).c_str(), nullptr); });
        }
        else
        {
            webViewWindow->ExecuteScript(
                widen(formattedCode).c_str(),
                Microsoft::WRL::Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
                    []([[maybe_unused]] HRESULT errorCode, [[maybe_unused]] LPCWSTR resultObjectAsJson) -> HRESULT {
                        return S_OK;
                    })
                    .Get());
        }
    }

    void WebView2::hide()
    {
        isHidden = true;
        ShowWindow(hwnd, SW_HIDE);
    }

    void WebView2::show()
    {
        isHidden = false;
        ShowWindow(hwnd, SW_SHOW);
        SetFocus(hwnd);
    }

    void WebView2::exit()
    {
        PostMessage(hwnd, WM_QUIT, 0, 0);
    }

    void WebView2::runThreadSafe(std::function<void()> func)
    {
        auto *call = new ThreadSafeCall{func};
        PostMessage(hwnd, WM_CALL, reinterpret_cast<ULONG_PTR>(call), 0);
    }
} // namespace Soundux
#endif