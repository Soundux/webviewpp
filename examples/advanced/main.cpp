#include <iostream>
#include <javascript/function.hpp>
#include <webview.hpp>

int main()
{
    Webview::Window webview(800, 900);
    webview.setTitle("Example");
    webview.enableDevTools(true);
    webview.enableContextMenu(true);

    webview.expose(Webview::Function("test", [](int someInt) { return someInt + 10; }));

    webview.expose(Webview::AsyncFunction("asyncTest", [](Webview::Promise promise, int someInt) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        promise.resolve(someInt + 10);
    }));

    webview.expose(Webview::AsyncFunction("pow", [&webview](Webview::Promise promise, int a, int b) {
        //! You should never call a javascript function in a non async context!
        auto pow = webview.callFunction<long>(Webview::JavaScriptFunction("Math.pow", a, b));
        promise.resolve(pow.get());
    }));

    webview.setCloseCallback([] {
        //* If we return `true` the window will not close!
        return true;
    });

    webview.setUrl("https://ddg.gg");
    webview.run();

    return 0;
}