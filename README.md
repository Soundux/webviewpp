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

## Dependencies
- Windows
  - (Runtime) [Webview2](https://docs.microsoft.com/microsoft-edge/webview2/) or Edge Chromium Canary Build
- Linux
  - (Runtime & Build) webkit2gtk

## Embedding
webviewpp supports embedding of all required files.  
To embed your files you have to use the [embed-helper](https://github.com/Soundux/webviewpp/tree/master/embed-helper).

Usage:
  - Compile the embed-helper
    - ```bash
      mkdir build && cd build && cmake .. && cmake --build . --config Release
      ```
  - Run the embed-helper
    - ```bash
      ./embed_helper <path to folder containing all the required files>
      ```
  - Add the parent folder of the `embedded` folder to your include directories
  - Change `navigate` calls to
    - `embedded://<filepath>` on Linux
    - `file:///embedded/<filepath>` on Windows

> For an example see [examples/embedded](https://github.com/Soundux/webviewpp/tree/master/examples/embedded)

## Documentation
The documentation was moved to the [main header file](https://github.com/Soundux/webviewpp/blob/master/src/webview/webview.hpp#L200)


<i>Note: This work was originally based on the work of [MichaelKim](https://github.com/MichaelKim/webview)</i>
