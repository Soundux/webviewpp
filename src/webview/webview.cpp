#include "webview.hpp"
#include <cstdint>
#include <memory>
#include <string>

namespace Soundux
{
    std::uint64_t WebView::seq = 0;
    void WebView::setSize(int width, int height)
    {
        this->width = width;
        this->height = height;
    }
    void WebView::navigate(const std::string &url)
    {
        this->url = url;
    }
    bool WebView::getDevToolsEnabled()
    {
        return devTools;
    }
    void WebView::enableDevTools(bool state)
    {
        devTools = state;
    }
    void WebView::setResizeCallback(const std::function<void(int, int)> &callback)
    {
        resizeCallback = callback;
    }
    void WebView::setNavigateCallback(const std::function<void(const std::string &)> &callback)
    {
        navigateCallback = callback;
    }
    void WebView::resolveCallback(const std::string &data)
    {
        auto j = nlohmann::json::parse(data, nullptr, false);
        if (!j.is_discarded())
        {
            auto seq = j.at("seq").get<std::uint32_t>();
            auto name = j.at("name").get<std::string>();

            if (name == "nativeCall")
            {
                if (j.find("result") != j.end())
                {
                    auto result = j.at("result");
                    nativeCalls.at(seq)(result);
                    nativeCalls.erase(seq);
                }
                else
                {
                    nativeCalls.erase(seq);
                }
                if (nativeCalls.empty())
                {
                    if (whenAllReadyCallback)
                    {
                        whenAllReadyCallback();
                    }
                }
            }
            else
            {
                auto params = j.at("params");

                const auto &callback = callbacks.at(name);

                auto *syncPtr = dynamic_cast<SyncCallback *>(callback.get());
                if (syncPtr)
                {
                    auto code = std::regex_replace(resolve_code, std::regex(R"(\{0\})"), std::to_string(seq));
                    code = std::regex_replace(code, std::regex(R"(\{1\})"), syncPtr->function(params));
                    runCode(code);
                }
                else
                {
                    dynamic_cast<AsyncCallback *>(callback.get())->function(params, seq);
                }
            }
        }
    }
    void WebView::onResize(int width, int height)
    {
        if (resizeCallback)
        {
            resizeCallback(width, height);
        }
    }
    void WebView::onExit()
    {
        shouldExit = true;
    }
    void WebView::onNavigate(const std::string &url)
    {
        if (navigateCallback)
        {
            navigateCallback(url);
        }
    }
    void WebView::runCodeSafe(const std::string &code)
    {
        runThreadSafe([=] { runCode(code); });
    }
    void WebView::hideOnClose(bool state)
    {
        shouldHideOnExit = state;
    }

    void WebView::onClosed()
    {
        if (closeCallback)
        {
            closeCallback();
        }
    }

    void WebView::setCloseCallback(const std::function<void()> &func)
    {
        closeCallback = func;
    }

    bool WebView::getIsHidden()
    {
        return isHidden;
    }

    void WebView::whenAllReady(const std::function<void()> &func)
    {
        whenAllReadyCallback = func;
    }
} // namespace Soundux
