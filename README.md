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
    add_subdirectory(/path/to/webviewpp EXCLUDE_FROM_ALL)
    link_libraries(webview)
    ```
- Use the library
  - See [documentation](#documentation)
  - See `examples` for examples

## Documentation
The documentation was moved to the [main header file](https://github.com/Soundux/webviewpp/blob/master/src/webview/webview.hpp#L200)


<i>Note: This work was originally based on the work of [MichaelKim](https://github.com/MichaelKim/webview)</i>
