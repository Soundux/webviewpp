#pragma once
#include <string>

namespace Webview
{
    // TODO(embedding): Implement embedding, update embed helper
    struct Resource
    {
        const std::size_t size;
        const unsigned char *data;
    };
} // namespace Webview