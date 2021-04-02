#include <webview.hpp>

int main()
{
    SounduxWebView webview;
    webview.setup(800, 900);
    webview.setTitle("Embedded Example");
    webview.enableDevTools(true);

    webview.navigate("embedded:///index.html");
    webview.run();

    return 0;
}