#include <webview.hpp>

int main()
{
    Webview::Window webview("webview", 800, 900);
    webview.setTitle("Embedded Example");
    webview.enableDevTools(true);

    webview.setUrl("file:///embedded/index.html");
    webview.run();

    return 0;
}