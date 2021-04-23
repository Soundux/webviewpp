#include <core/basewindow.hpp>
#include <exception>
#include <javascript/promise.hpp>
#include <json/bindings.hpp>
#include <regex>

const std::string Webview::BaseWindow::callbackFunctionDefinition = R"js(
async function {0}(...param)
{
    const seq = ++window._rpc_seq;
    const promise = new Promise((resolve) => {
        window._rpc[seq] = {
            resolve: resolve
        };
    });
    window.external.invoke(JSON.stringify({
        "seq": seq,
        "params": param,
        "function": "{0}"
    }));
    return JSON.parse(await promise);
}
)js";
const std::string Webview::BaseWindow::setupRpc = "window._rpc = {}; window._rpc_seq = 0;";
const std::string Webview::BaseWindow::resolveCall = R"js(
    window._rpc[{0}].resolve(`{1}`);
    delete window._rpc[{0}];
)js";
const std::string Webview::BaseWindow::resolveNativeCall = R"js(
window.external.invoke(JSON.stringify({
    "seq": {0},
    "result": {1}
}));
)js";

Webview::BaseWindow::BaseWindow(std::string identifier, std::size_t width, std::size_t height)
    : width(width), height(height), identifier(std::move(identifier))
{
}

bool Webview::BaseWindow::onClose()
{
    if (closeCallback)
    {
        return closeCallback();
    }
    return false;
}

void Webview::BaseWindow::onResize(std::size_t width, std ::size_t height)
{
    if (resizeCallback)
    {
        resizeCallback(width, height);
    }
}

void Webview::BaseWindow::onNavigate(std::string newUrl)
{
    url = std::move(newUrl);

    if (navigateCallback)
    {
        navigateCallback(url);
    }
}

void Webview::BaseWindow::handleRawCallRequest(const std::string &rawRequest)
{
    auto parsed = nlohmann::json::parse(rawRequest, nullptr, false);
    if (!parsed.is_discarded())
    {
        if (parsed.find("result") != parsed.end())
        {
            auto request = parsed.get<NativeCallResponse>();
            auto &function = nativeCallRequests.at(request.seq);
            function.resolve(request.result);
        }
        else
        {
            auto request = parsed.get<FunctionCallRequest>();
            const auto &function = functions.at(request.function);

            if (const auto *asyncFunction = dynamic_cast<const AsyncFunction *>(function.get()); asyncFunction)
            {
                auto future = std::make_shared<std::future<void>>();
                *future = std::async(std::launch::async, [future, request, asyncFunction, this]() {
                    asyncFunction->getFunc()(*this, request.params, request.seq);
                });
            }
            else
            {
                const auto &result = function->getFunc()(request.params);

                auto responseCode =
                    std::regex_replace(resolveCall, std::regex(R"(\{0\})"), std::to_string(request.seq));
                responseCode = std::regex_replace(responseCode, std::regex(R"(\{1\})"), result.dump());
                runCode(responseCode);
            }
        }
    }
}

void Webview::BaseWindow::hide()
{
    hidden = true;
}

void Webview::BaseWindow::show()
{
    hidden = false;
}

bool Webview::BaseWindow::isHidden()
{
    return hidden;
}

void Webview::BaseWindow::setSize(std::size_t newWidth, std::size_t newHeight)
{
    width = newWidth;
    height = newHeight;
}

std::pair<std::size_t, std::size_t> Webview::BaseWindow::getSize()
{
    return std::make_pair(width, height);
}

std::string Webview::BaseWindow::getTitle()
{
    return title;
}

void Webview::BaseWindow::setTitle(std::string newTitle)
{
    title = std::move(newTitle);
}

std::string Webview::BaseWindow::getUrl()
{
    return url;
}

void Webview::BaseWindow::setUrl(std::string newUrl)
{
    url = std::move(newUrl);
}

void Webview::BaseWindow::setCloseCallback(std::function<bool()> callback)
{
    closeCallback = std::move(callback);
}

void Webview::BaseWindow::setNavigateCallback(std::function<void(const std::string &)> callback)
{
    navigateCallback = std::move(callback);
}

void Webview::BaseWindow::setResizeCallback(std::function<void(std::size_t, std::size_t)> callback)
{
    resizeCallback = std::move(callback);
}

void Webview::BaseWindow::expose(const Function &function)
{
    std::shared_ptr<Function> ptr;

    try
    {
        const auto &asyncFunction = dynamic_cast<const AsyncFunction &>(function);
        ptr = std::make_shared<AsyncFunction>(asyncFunction);
    }
    catch (const std::exception &e)
    {
        ptr = std::make_shared<Function>(function);
    }

    functions.emplace(function.getName(), ptr);
    injectCode(std::regex_replace(callbackFunctionDefinition, std::regex(R"(\{0\})"), function.getName()));
}

std::string Webview::BaseWindow::formatCode(const std::string &code)
{
    auto formattedCode = std::regex_replace(code, std::regex(R"rgx(\\")rgx"), R"(\\\")");
    formattedCode = std::regex_replace(formattedCode, std::regex(R"rgx(\\n)rgx"), R"(\\n)");
    formattedCode = std::regex_replace(formattedCode, std::regex(R"rgx(\\t)rgx"), R"(\\t)");

    return formattedCode;
}

Webview::Resource Webview::BaseWindow::getResource(const std::string & /*name*/)
{
    return {};
}

void Webview::BaseWindow::enableContextMenu(bool state)
{
    isContextMenuAllowed = state;
}

Webview::JavaScriptFunction &Webview::BaseWindow::callFunction(Webview::JavaScriptFunction &&function)
{
    static std::uint32_t seq = 0;

    auto call = function.getName() + "(";
    for (const auto &argument : function.getArguments())
    {
        call += argument.dump() + ",";
    }
    if (!function.getArguments().empty())
    {
        call.back() = ')';
    }
    else
    {
        call += ")";
    }

    auto code = std::regex_replace(resolveNativeCall, std::regex(R"(\{0\})"), std::to_string(++seq));
    code = std::regex_replace(code, std::regex(R"(\{1\})"), call);
    runCode(code);

    nativeCallRequests.emplace(seq, function);
    return nativeCallRequests.at(seq);
}