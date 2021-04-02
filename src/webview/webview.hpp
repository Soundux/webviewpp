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
      protected:
        struct ThreadSafeCall
        {
            std::function<void()> func;
        };
        template <typename T> class Future
        {
            friend class WebView;

            std::optional<T> rtn;
            std::function<void(const T &)> callback;

            void resolve(T result)
            {
                rtn = std::move(result);
                if (callback)
                {
                    callback(*rtn);
                }
            }

          public:
            void then(std::function<void(const T &)> then)
            {
                callback = std::move(then);
            }
        };

        struct Callback
        {
            std::string code;

            Callback(std::string code) : code(std::move(code)) {}
            virtual ~Callback() = default;
        };
        struct SyncCallback : Callback
        {
            std::function<std::string(const nlohmann::json &)> function;

            SyncCallback(const std::string &code, std::function<std::string(const nlohmann::json &)> function)
                : Callback(code), function(std::move(function))
            {
            }
            ~SyncCallback() override = default;
        };
        struct AsyncCallback : Callback
        {
            std::function<void(const nlohmann::json &, const std::uint32_t &)> function;

            AsyncCallback(const std::string &code,
                          std::function<void(const nlohmann::json &, const std::uint32_t &)> function)
                : Callback(code), function(std::move(function))
            {
            }
            ~AsyncCallback() override = default;
        };

        int width;
        int height;
        bool devTools;
        std::string url;
        bool shouldExit;
        bool isHidden = false;
        static std::uint64_t seq;
        bool shouldHideOnExit = false;
        std::function<void()> closeCallback;
        std::function<void()> whenAllReadyCallback;
        std::function<void(int, int)> resizeCallback;
        std::function<void(const std::string &)> navigateCallback;
        std::map<std::string, std::unique_ptr<Callback>> callbacks;
        std::map<std::uint64_t, std::function<void(const nlohmann::json &)>> nativeCalls;

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
        static inline std::string native_code = R"js(
            window.external.invoke(JSON.stringify({
                "seq": {0},
                "name": "nativeCall",
                "result": {1}
            }));
        )js";
        static inline std::string resolve_code = R"js(
            window._rpc[{0}].resolve(`{1}`);
            delete window._rpc[{0}];
        )js";
        static inline std::string setup_code = R"js(
            window._rpc = {}; window._rpc_seq = 0;
        )js";

        virtual void onExit();
        virtual void onClosed();
        virtual void onResize(int, int);
        virtual void onNavigate(const std::string &);
        virtual void resolveCallback(const std::string &);
        virtual std::pair<std::size_t, unsigned char *> getEmbeddedResource(const std::string &) const;

      public:
        WebView() = default;
        virtual ~WebView() = default;
        WebView(const WebView &) = delete;
        virtual WebView &operator=(const WebView &) = delete;

        /// @brief Returns wether or not the devtools are enabled
        virtual bool getDevToolsEnabled();
        /// @brief Enables or disables the devtools
        /// @remarks Will also disable the context-menu when devtools are disabled
        virtual void enableDevTools(bool);
        /// @brief Returns wether or not the window is hidden
        virtual bool getIsHidden();

        /// @brief Runs the webview
        /// @remarks Blocks until the webview is closed
        virtual void run() = 0;
        /// @brief Will show the window
        virtual void show() = 0;
        /// @brief Will hide the window
        virtual void hide() = 0;
        /// @brief Will close the window and finish execution
        virtual void exit() = 0;
        /// @brief This function will setup the webview
        /// @remarks Is required to be run first
        /// @param width Width of the window to be created
        /// @param height Height of the window to be created
        virtual bool setup(int, int) = 0;
        /// @brief Will run the given function in a thread safe manner
        virtual void runThreadSafe(std::function<void()>) = 0;

        /// @brief Will set the windows size
        virtual void setSize(int, int);
        /// @brief If this is set to `true` the window will hide itself instead of closing
        virtual void hideOnClose(bool);
        /// @brief Will navigate to the given url
        virtual void navigate(const std::string &);
        /// @brief Will set the windows title
        virtual void setTitle(const std::string &) = 0;
        /// @brief Will run the given code thread safe
        virtual void runCodeSafe(const std::string &);
        /// @brief Will run the given javascript code
        /// @remarks this may crash the webview if it's not run thread safe
        virtual void runCode(const std::string &, bool = false) = 0;

        /// @brief Will run the given function as soon as all calls from `callJS` finished
        virtual void whenAllReady(const std::function<void()> &);
        /// @brief Will call the given callback as soon as the window is closed
        /// @remarks Will also get called when the window gets hidden (should `hideOnClose` be `true`)
        virtual void setCloseCallback(const std::function<void()> &);
        /// @brief Will call the given callback when the window gets resized
        virtual void setResizeCallback(const std::function<void(int, int)> &);
        /// @brief Will be called on a navigation (url changed) event
        virtual void setNavigateCallback(const std::function<void(const std::string &)> &);

        /// @brief Will resolve the given promise with the given result
        /// @remarks The given result has to be serializeable by `nlohmann::json`
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
        /// @brief Will call a JavaScript function and return its result as `rtn_t`
        /// @remarks `rtn_t` as well as all `T` have to be serializeable by `nlohmann::json`
        /// @param function Name of the javascript function, e.g. `console.log`
        /// @param parameters Parameter list of the parameters that should be given to the given js function
        /// @returns Future that will be resolved when the given javascript function finished executing
        template <typename rtn_t, typename... T>
        [[nodiscard]] std::shared_ptr<Future<rtn_t>> callJS(const std::string &function, const T &...parameters)
        {
            auto funcCall = function + "(";
            static auto unpackArgsFn = [&](const auto &arg) {
                if constexpr (traits::is_optional<std::decay_t<decltype(arg)>>::value)
                {
                    if (arg)
                    {
                        funcCall += nlohmann::json(arg).dump();
                    }
                    else
                    {
                        funcCall += "null";
                    }
                }
                else
                {
                    funcCall += nlohmann::json(arg).dump();
                }
                funcCall += ",";
            };

            if constexpr (sizeof...(parameters) > 0)
            {
                (unpackArgsFn(parameters), ...);
                funcCall.back() = ')';
            }
            else
            {
                funcCall += ")";
            }

            seq++;
            auto rtn = std::make_shared<Future<rtn_t>>();
            nativeCalls.emplace(seq, [rtn](const nlohmann::json &j) { rtn->resolve(j.get<rtn_t>()); });

            auto code = std::regex_replace(native_code, std::regex(R"(\{0\})"), std::to_string(seq));
            code = std::regex_replace(code, std::regex(R"(\{1\})"), funcCall);
            runCodeSafe(code);

            return rtn;
        }
        /// @brief Will register the given function as a javascript function
        /// @param name The name the javascript function to be created
        /// @param function The function to be turned into javascript function
        /// @remarks All parameters as well as the return type of `func_t` have to be serializable by `nlohmann::json`
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

                callbacks.emplace(name, std::make_unique<AsyncCallback>(code, func));
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

                callbacks.emplace(name, std::make_unique<SyncCallback>(code, func));
            }
        }
    };
} // namespace Soundux