## webviewpp
A cross-platform C++17 library that allows you to create a simple webview.

## Compatibility
| Platform | Used Browser                                                                    | GUI    |
| -------- | ------------------------------------------------------------------------------- | ------ |
| Windows  | [Webview2](https://docs.microsoft.com/microsoft-edge/webview2/) (Edge Chromium) | WinAPI |
| Linux    | WebKit2GTK                                                                      | GTK    |

## Usage

- Add the library to your project
  - ```cmake
    add_subdirectory(/path/to/webviewpp)
    link_libraries(webview)
    ```
- Use the library
  - See [documentation](#documentation)
  - See `examples` for examples

## Documentation

```cpp
bool WebView::getDevToolsEnabled();
```
`Brief:` <i>Returns if dev tools are enabled</i>

---
```cpp
bool WebView::enableDevTools(bool enable);
```
`Remarks:` <i>Disabling devtools also disables context menu</i>  
`Enable:` <i>False disables devtools, true enables devtools</i>  
`Brief:` <i>Toggles the devtools</i>  

---
```cpp
bool WebView::run();
```
`Returns:`
- `true` if the ui should not exit  
- `false` if the ui should exit
  
`Brief:` <i>Main-Loop</i>

---
```cpp
bool WebView::setup(int width, int height);
```
`Brief:` <i>Initializes the webview</i>  
`Returns:` <i>`false` if the creation failed</i>

---
```cpp
void WebView::setSize(int width, int height);
```
`Brief:` <i>Sets the windows size</i>

---
```cpp
void WebView::navigate(const std::string &url);
```
`Brief:` <i>Navigates to the given url</i>

---
```cpp
void WebView::runCode(const std::string &code);
```
`Code:` <i>The javascript code to run</i>  
`Brief:` <i>Runs the given code</i>

---
```cpp
void WebView::setTitle(const std::string &title);
```
`Brief:` <i>Sets the windows title</i>

---
```cpp
void WebView::setResizeCallback(const std::function<void(int, int)> &callback);
```
`Callback:` <i>The callback to call when the window is resized</i>  
`Brief:` <i>Sets the resize callback</i>

---
```cpp
void WebView::setNavigateCallback(const std::function<void(const std::string &)> &callback);
```
`Callback:` <i>The callback to call when the webview navigates</i>  
`Brief:` <i>Sets the navigate callback</i>

---
```cpp
template <typename func_t>
void WebView::addCallback(const std::string &name, func_t function);
```
`Brief:` <i>Adds a binding</i>  
`Name:` <i>Name of the function to be added</i>  
`Remarks:`   
<i>The return type of the function must be default constructible or void</i>  
<i>All of the functions parameters as well as the functions return type need to be [serializable by nlohmann::json](https://github.com/nlohmann/json#how-do-i-convert-third-party-types)</i>  
<i>The return type can also be `std::optional` in that case the javascript function will return `null` if  the optional doesn't hold a value</i>


<i>Note: This work was originally based on the work of [MichaelKim](https://github.com/MichaelKim/webview)</i>