#include <iostream>
#include <javascript/function.hpp>
#include <webview.hpp>

int main()
{
    Webview::Window webview(800, 900);
    webview.setTitle("Example");
    webview.enableDevTools(true);

    // Call me using the dev tools!
    webview.expose(Webview::Function("testCallback", [](const std::string &someString, int someInt) {
        std::cout << "Got " << someString << " and " << someInt;
        return someInt * 10;
    }));

    webview.setUrl("https://ddg.gg");
    webview.run();

    return 0;
}