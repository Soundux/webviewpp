#if defined(__linux__)
#include "webkit2gtk.hpp"

namespace Soundux
{
    void WebKit2Gtk::messageReceived([[maybe_unused]] WebKitUserContentManager *contentManager,
                                     WebKitJavascriptResult *result, gpointer arg)
    {
        auto *webview = reinterpret_cast<WebKit2Gtk *>(arg);
        if (!webview->callbacks.empty())
        {
            auto *value = webkit_javascript_result_get_js_value(result);
            auto str = std::string(jsc_value_to_string(value));
            webview->resolveCallback(str);
        }
    }

    void WebKit2Gtk::loadChanged(WebKitWebView *webkitwebview, [[maybe_unused]] WebKitLoadEvent event,
                                 [[maybe_unused]] gpointer arg)
    {
        if (event == WEBKIT_LOAD_FINISHED)
        {
            auto *webview = reinterpret_cast<WebKit2Gtk *>(arg);
            webview->onNavigate(webkit_web_view_get_uri(webkitwebview));
        }
    }

    gboolean WebKit2Gtk::resize([[maybe_unused]] WebKitWebView *webkitwebview, GdkEvent *event, gpointer arg)
    {
        auto *gtkEvent = reinterpret_cast<GdkEventConfigure *>(event);
        auto *webview = reinterpret_cast<WebKit2Gtk *>(arg);

        webview->onResize(gtkEvent->width, gtkEvent->height);
        return 0;
    }

    void WebKit2Gtk::destroy([[maybe_unused]] GtkWidget *widget, gpointer arg)
    {
        auto *webview = reinterpret_cast<WebKit2Gtk *>(arg);
        webview->onExit();
    }

    void WebKit2Gtk::runThreadSafe(std::function<void()> func)
    {
        auto *call = new ThreadSafeCall{func};

        g_idle_add(
            +[](gpointer data) -> gboolean {
                auto *threadSafeCall = reinterpret_cast<ThreadSafeCall *>(data);
                threadSafeCall->func();
                delete threadSafeCall;
                return FALSE;
            },
            call);
    }

    void WebKit2Gtk::hide()
    {
        isHidden = true;
        gtk_widget_hide(window);
    }

    void WebKit2Gtk::show()
    {
        isHidden = false;
        gtk_widget_show(window);
    }

    gboolean WebKit2Gtk::contextMenu([[maybe_unused]] WebKitWebView *webkitwebview, [[maybe_unused]] GtkWidget *widget,
                                     [[maybe_unused]] WebKitHitTestResultContext *result,
                                     [[maybe_unused]] gboolean with_keyboard, gpointer arg)
    {
        auto *webview = reinterpret_cast<WebKit2Gtk *>(arg);
        if (webview->getDevToolsEnabled())
        {
            return 0;
        }
        return 1;
    }

    gboolean WebKit2Gtk::onClose([[maybe_unused]] GtkWidget *widget, [[maybe_unused]] GdkEvent *event, gpointer data)
    {
        auto *webview = reinterpret_cast<WebKit2Gtk *>(data);

        if (webview->shouldHideOnExit)
        {
            webview->hide();
            webview->onClosed();

            return TRUE;
        }

        webview->onClosed();
        return FALSE;
    }

    bool WebKit2Gtk::setup(int width, int height)
    {
        this->width = width;
        this->height = height;

        if (gtk_init_check(nullptr, nullptr) == 0)
        {
            return false;
        }

        window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_resizable(GTK_WINDOW(window), true);              // NOLINT
        gtk_window_set_default_size(GTK_WINDOW(window), width, height);  // NOLINT
        gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER); // NOLINT

        GtkWidget *scrollView = gtk_scrolled_window_new(nullptr, nullptr);
        gtk_container_add(GTK_CONTAINER(window), scrollView); // NOLINT

        WebKitUserContentManager *contentManager = webkit_user_content_manager_new();
        webkit_user_content_manager_register_script_message_handler(contentManager, "external");

        webview = webkit_web_view_new_with_user_content_manager(contentManager);
        gtk_container_add(GTK_CONTAINER(scrollView), webview); // NOLINT

        // NOLINTNEXTLINE
        g_signal_connect(contentManager, "script-message-received::external", G_CALLBACK(messageReceived), this);
        g_signal_connect(G_OBJECT(webview), "load-changed", G_CALLBACK(loadChanged), this); // NOLINT
        g_signal_connect(G_OBJECT(webview), "context-menu", G_CALLBACK(contextMenu), this); // NOLINT
        g_signal_connect(window, "configure-event", G_CALLBACK(resize), this);              // NOLINT
        g_signal_connect(window, "delete_event", G_CALLBACK(onClose), this);                // NOLINT
        g_signal_connect(window, "destroy", G_CALLBACK(destroy), this);                     // NOLINT

        gtk_widget_grab_focus(GTK_WIDGET(webview)); // NOLINT
        gtk_widget_show_all(window);

        runCode("window.external={invoke:arg=>window.webkit."
                "messageHandlers.external.postMessage(arg)};",
                true);
        runCode(setup_code, true);

        return true;
    }
    void WebKit2Gtk::run()
    {
        while (!shouldExit)
        {
            gtk_main_iteration_do(true);
        }
    }
    void WebKit2Gtk::setTitle(const std::string &title)
    {
        assert(window != nullptr);
        gtk_window_set_title(GTK_WINDOW(window), title.c_str()); // NOLINT
    }
    void WebKit2Gtk::setSize(int width, int height)
    {
        assert(window != nullptr);
        gtk_window_resize(GTK_WINDOW(window), width, height); // NOLINT
    }
    void WebKit2Gtk::navigate(const std::string &url)
    {
        WebView::navigate(url);

        assert(webview != nullptr);
        webkit_web_view_load_uri(WEBKIT_WEB_VIEW(webview), url.c_str()); // NOLINT
    }
    void WebKit2Gtk::enableDevTools(bool enable)
    {
        WebView::enableDevTools(enable);

        assert(webview != nullptr);
        WebKitSettings *settings = webkit_web_view_get_settings(WEBKIT_WEB_VIEW(webview)); // NOLINT
        webkit_settings_set_enable_write_console_messages_to_stdout(settings, enable);
        webkit_settings_set_enable_developer_extras(settings, enable);
    }
    void WebKit2Gtk::runCode(const std::string &code, bool inject)
    {
        assert(webview != nullptr);

        auto formattedCode = std::regex_replace(code, std::regex(R"rgx(\\")rgx"), R"(\\\")");
        formattedCode = std::regex_replace(formattedCode, std::regex(R"rgx(\\n)rgx"), R"(\\n)");
        formattedCode = std::regex_replace(formattedCode, std::regex(R"rgx(\\t)rgx"), R"(\\t)");

        // NOLINTNEXTLINE
        if (inject)
        {
            auto *manager = webkit_web_view_get_user_content_manager(WEBKIT_WEB_VIEW(webview)); // NOLINT
            webkit_user_content_manager_add_script(
                manager, webkit_user_script_new(formattedCode.c_str(), WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
                                                WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START, nullptr, nullptr));
        }
        else
        {
            // NOLINTNEXTLINE
            webkit_web_view_run_javascript(WEBKIT_WEB_VIEW(webview), formattedCode.c_str(), nullptr, nullptr, nullptr);
        }
    }
    void WebKit2Gtk::exit()
    {
        gtk_widget_destroy(reinterpret_cast<GtkWidget *>(window));
    }
} // namespace Soundux
#endif