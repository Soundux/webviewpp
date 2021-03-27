#pragma once
#include <atomic>
#include <cstdint>
#include <functional>
#include <json.hpp>
#include <map>
#include <mutex>
#include <optional>
#include <queue>
#include <regex>
#include <string>
#include <type_traits>

namespace Soundux
{
    namespace traits
    {
        template <typename T> struct func_traits : public func_traits<decltype(&T::operator())>
        {
        };
        template <typename ClassType, typename ReturnType, typename... Args>
        struct func_traits<ReturnType (ClassType::*)(Args...) const>
        {
            enum
            {
                arg_count = sizeof...(Args)
            };
            using arg_t = std::tuple<std::decay_t<Args>...>;
            using return_t = ReturnType;
        };
        template <typename T> struct is_optional
        {
          private:
            static std::uint8_t test(...);
            template <typename O> static auto test(std::optional<O> *) -> std::uint16_t;

          public:
            static const bool value = sizeof(test(reinterpret_cast<T *>(0))) == sizeof(std::uint16_t);
        };

        template <typename T, typename Tuple> struct has_type;
        template <typename T> struct has_type<T, std::tuple<>> : std::false_type
        {
        };
        template <typename T, typename U, typename... Ts>
        struct has_type<T, std::tuple<U, Ts...>> : has_type<T, std::tuple<Ts...>>
        {
        };
        template <typename T, typename... Ts> struct has_type<T, std::tuple<T, Ts...>> : std::true_type
        {
        };
    } // namespace traits
    namespace helpers
    {
        template <std::size_t I, typename Tuple, typename Function, std::enable_if_t<(I >= 0)> * = nullptr>
        void setTupleImpl(Tuple &tuple, Function func)
        {
            if constexpr (I >= 0)
            {
                func(I, std::get<I>(tuple));
                if constexpr (I > 0)
                {
                    setTupleImpl<I - 1>(tuple, func);
                }
            }
        }
        template <int I, typename Tuple, typename Function> void setTuple(Tuple &tuple, Function func)
        {
            if constexpr (I >= 0)
            {
                setTupleImpl<I>(tuple, func);
            }
        }
    } // namespace helpers

    struct JSPromise
    {
        std::uint32_t id;
    };

    class WebView
    {
        struct callback
        {
            std::string code;

            callback(std::string code) : code(std::move(code)) {}
            virtual ~callback() = default;
        };
        struct syncCallback : callback
        {
            std::function<std::string(const nlohmann::json &)> function;

            syncCallback(const std::string &code, std::function<std::string(const nlohmann::json &)> function)
                : callback(code), function(std::move(function))
            {
            }
            ~syncCallback() override = default;
        };
        struct asyncCallback : callback
        {
            std::function<void(const nlohmann::json &, const std::uint32_t &)> function;

            asyncCallback(const std::string &code,
                          std::function<void(const nlohmann::json &, const std::uint32_t &)> function)
                : callback(code), function(std::move(function))
            {
            }
            ~asyncCallback() override = default;
        };

      protected:
        int width;
        int height;
        bool devTools;
        std::string url;
        bool shouldExit;

        std::mutex queueMutex;
        std::atomic<bool> checkQueue;
        std::queue<std::string> codeQueue;

        std::function<void(int, int)> resizeCallback;
        std::function<void(const std::string &)> navigateCallback;
        std::map<std::string, std::unique_ptr<callback>> callbacks;

        static inline std::string callback_code = R"js(
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
                        "name": "{0}",
                        "params": param
                    }));
                    return JSON.parse(await promise);
                }
        )js";
        static inline std::string resolve_code = R"js(
            window._rpc[{0}].resolve(`{1}`);
            delete window._rpc[{0}];
        )js";
        static inline std::string setup_code = R"js(
            window._rpc = {}; window._rpc_seq = 0;
        )js";

        virtual void onExit();
        virtual void doQueue();
        virtual void onResize(int, int);
        virtual void onNavigate(const std::string &);
        virtual void resolveCallback(const std::string &);

      public:
        WebView() = default;
        virtual ~WebView() = default;
        WebView(const WebView &) = delete;
        virtual WebView &operator=(const WebView &) = delete;

        virtual bool getDevToolsEnabled();
        virtual void enableDevTools(bool);

        virtual bool run() = 0;
        virtual bool setup(int, int) = 0;

        virtual void setSize(int, int);
        virtual void navigate(const std::string &);
        virtual void setTitle(const std::string &) = 0;

        virtual void runCodeSafe(const std::string &);
        virtual void runCode(const std::string &, bool = false) = 0;

        virtual void setResizeCallback(const std::function<void(int, int)> &);
        virtual void setNavigateCallback(const std::function<void(const std::string &)> &);

        template <typename T> void resolve(const JSPromise &promise, const T &result)
        {
            std::string rtn;
            if constexpr (traits::is_optional<std::decay_t<T>>::value)
            {
                if (result)
                {
                    rtn = nlohmann::json(*result).dump();
                }
                else
                {
                    rtn = "null";
                }
            }
            else
            {
                rtn = nlohmann::json(result).dump();
            }

            auto code = std::regex_replace(resolve_code, std::regex(R"(\{0\})"), std::to_string(promise.id));
            code = std::regex_replace(code, std::regex(R"(\{1\})"), rtn);
            runCodeSafe(code);
        }
        template <typename func_t> void addCallback(const std::string &name, func_t function)
        {
            using func_traits = traits::func_traits<decltype(function)>;
            using arg_t = typename func_traits::arg_t;

            if constexpr (traits::has_type<JSPromise, arg_t>::value && std::is_void_v<typename func_traits::return_t>)
            {
                auto func = [this, function](const nlohmann::json &j, const std::uint32_t &seq) {
                    arg_t packedArgs;

                    helpers::setTuple<func_traits::arg_count - 2>(
                        packedArgs, [&j](auto index, auto &val) { j.at(index).get_to(val); });
                    std::get<func_traits::arg_count - 1>(packedArgs) = JSPromise{seq};

                    auto unpackFunc = [seq, function](auto &&...args) { function(args...); };
                    std::apply(unpackFunc, packedArgs);
                };

                auto code = std::regex_replace(callback_code, std::regex(R"(\{0\})"), name);
                runCode(code, true);

                callbacks.emplace(name, std::make_unique<asyncCallback>(code, func));
            }
            else
            {
                auto func = [this, function](const nlohmann::json &j) -> std::string {
                    arg_t packedArgs;

                    helpers::setTuple<func_traits::arg_count - 1>(
                        packedArgs, [&j](auto index, auto &val) { j.at(index).get_to(val); });

                    if constexpr (std::is_void_v<typename func_traits::return_t>)
                    {
                        auto unpackFunc = [function](auto &&...args) { function(args...); };
                        std::apply(unpackFunc, packedArgs);
                        return "null";
                    }
                    else if constexpr (traits::is_optional<typename func_traits::return_t>::value)
                    {
                        typename func_traits::return_t rtn;

                        auto unpackFunc = [&rtn, function](auto &&...args) { rtn = std::move(function(args...)); };
                        std::apply(unpackFunc, packedArgs);

                        if (rtn)
                        {
                            return nlohmann::json(*rtn).dump();
                        }
                        return "null";
                    }
                    else
                    {
                        typename func_traits::return_t rtn;

                        auto unpackFunc = [&rtn, function](auto &&...args) { rtn = std::move(function(args...)); };
                        std::apply(unpackFunc, packedArgs);

                        return nlohmann::json(rtn).dump();
                    }
                };

                auto code = std::regex_replace(callback_code, std::regex(R"(\{0\})"), name);
                runCode(code, true);

                callbacks.emplace(name, std::make_unique<syncCallback>(code, func));
            }
        }
    };
} // namespace Soundux