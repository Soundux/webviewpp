#include <iostream>
#include <webview.hpp>

int main()
{
    SounduxWebView webview;
    webview.setup(800, 900);
    webview.setTitle("Example");
    webview.enableDevTools(true);

    // Call me using the dev tools!
    webview.addCallback("testCallback", [](const std::string &someString, int someInt) {
        std::cout << "Got " << someString << " and " << someInt;
        return someInt * 10;
    });
    webview.navigate("https://ddg.gg");

    while (webview.run())
    {
    }

    return 0;
}