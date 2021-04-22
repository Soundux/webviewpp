#include <future>
#include <javascript/function.hpp>
#include <json.hpp>

std::function<std::string(const nlohmann::json &)> Webview::Function::getFunc() const
{
    return parserFunction;
}

std::string Webview::Function::getName() const
{
    return name;
}

std::function<void(Webview::BaseWindow &, const nlohmann::json &, std::uint32_t)> Webview::AsyncFunction::getFunc()
    const
{
    return parserFunction;
}

std::vector<nlohmann::json> Webview::JavaScriptFunction::getArguments() const
{
    return arguments;
}

std::string Webview::JavaScriptFunction::getName() const
{
    return name;
}

void Webview::JavaScriptFunction::resolve(const nlohmann::json &result)
{
    this->result.set_value(result);
}

std::shared_future<nlohmann::json> Webview::JavaScriptFunction::getResult()
{
    return result.get_future();
}

Webview::JavaScriptFunction::JavaScriptFunction(JavaScriptFunction &other)
    : name(other.name), result(std::move(other.result)), arguments(std::move(other.arguments))
{
}