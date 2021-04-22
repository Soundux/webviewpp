#pragma once
#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <string>

#include "resource.hpp"
#include <javascript/function.hpp>

namespace Webview
{
    class Promise;
    class BaseWindow
    {
        friend class Promise;

      protected:
        std::size_t width;
        std::size_t height;

        std::string url;
        std::string title;
        std::string identifier; //* Only required on windows

        bool hidden = false;
        bool isContextMenuAllowed = true;

        std::function<bool()> closeCallback;
        std::function<void(const std::string &)> navigateCallback;
        std::function<void(std::size_t, std::size_t)> resizeCallback;

        static const std::string setupRpc;
        static const std::string resolveCall;
        static const std::string resolveNativeCall;
        static const std::string resolveNativeFunction;
        static const std::string callbackFunctionDefinition;

        std::map<std::string, std::shared_ptr<Function>> functions;
        std::map<std::uint32_t, JavaScriptFunction> nativeCallRequests;

      protected:
        virtual bool onClose();
        virtual void onNavigate(std::string);
        virtual void onResize(std::size_t, std::size_t);

        virtual Resource getResource(const std::string &);
        virtual std::string formatCode(const std::string &);
        virtual void handleRawCallRequest(const std::string &);
        JavaScriptFunction &callFunction(JavaScriptFunction &&);

      public:
        BaseWindow(const BaseWindow &) = delete;
        virtual BaseWindow &operator=(const BaseWindow &) = delete;
        BaseWindow(std::string identifier, std::size_t width, std::size_t height);

        virtual void hide();
        virtual void show();
        virtual bool isHidden();

        virtual void setSize(std::size_t, std::size_t);
        virtual std::pair<std::size_t, std::size_t> getSize();

        virtual std::string getTitle();
        virtual void setTitle(std::string);

        virtual void run() = 0;
        virtual void exit() = 0;

        virtual std::string getUrl();
        virtual void setUrl(std::string);

        virtual void enableContextMenu(bool);
        virtual void enableDevTools(bool) = 0;

        void expose(const Function &);
        template <typename T> std::future<T> callFunction(JavaScriptFunction &&function)
        {
            auto future = callFunction(std::forward<JavaScriptFunction>(function)).getResult();
            return std::async(std::launch::deferred, [future] { return future.get().get<T>(); });
        }

        virtual void runCode(const std::string &) = 0;
        virtual void injectCode(const std::string &) = 0;

        virtual void setCloseCallback(std::function<bool()>);
        virtual void setNavigateCallback(std::function<void(const std::string &)>);
        virtual void setResizeCallback(std::function<void(std::size_t, std::size_t)>);
    };
} // namespace Webview