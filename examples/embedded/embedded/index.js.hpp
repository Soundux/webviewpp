#pragma once
#include "webview_base.hpp"
namespace Webview::Embedded {
inline unsigned char embed_file_index_js[] = {0x66,0x75,0x6e,0x63,0x74,0x69,0x6f,0x6e,0x20,0x61,0x6c,0x6c,0x6f,0x77,0x44,0x72,0x6f,0x70,0x28,0x65,0x76,0x29,0x20,0x7b,0x0a,0x20,0x20,0x65,0x76,0x2e,0x70,0x72,0x65,0x76,0x65,0x6e,0x74,0x44,0x65,0x66,0x61,0x75,0x6c,0x74,0x28,0x29,0x3b,0x0a,0x7d,0x0a,0x0a,0x66,0x75,0x6e,0x63,0x74,0x69,0x6f,0x6e,0x20,0x64,0x72,0x61,0x67,0x28,0x65,0x76,0x29,0x20,0x7b,0x0a,0x20,0x20,0x65,0x76,0x2e,0x64,0x61,0x74,0x61,0x54,0x72,0x61,0x6e,0x73,0x66,0x65,0x72,0x2e,0x73,0x65,0x74,0x44,0x61,0x74,0x61,0x28,0x22,0x74,0x65,0x78,0x74,0x22,0x2c,0x20,0x65,0x76,0x2e,0x74,0x61,0x72,0x67,0x65,0x74,0x2e,0x69,0x64,0x29,0x3b,0x0a,0x7d,0x0a,0x0a,0x66,0x75,0x6e,0x63,0x74,0x69,0x6f,0x6e,0x20,0x64,0x72,0x6f,0x70,0x28,0x65,0x76,0x29,0x20,0x7b,0x0a,0x20,0x20,0x65,0x76,0x2e,0x70,0x72,0x65,0x76,0x65,0x6e,0x74,0x44,0x65,0x66,0x61,0x75,0x6c,0x74,0x28,0x29,0x3b,0x0a,0x20,0x20,0x76,0x61,0x72,0x20,0x64,0x61,0x74,0x61,0x20,0x3d,0x20,0x65,0x76,0x2e,0x64,0x61,0x74,0x61,0x54,0x72,0x61,0x6e,0x73,0x66,0x65,0x72,0x2e,0x67,0x65,0x74,0x44,0x61,0x74,0x61,0x28,0x22,0x74,0x65,0x78,0x74,0x22,0x29,0x3b,0x0a,0x20,0x20,0x65,0x76,0x2e,0x74,0x61,0x72,0x67,0x65,0x74,0x2e,0x61,0x70,0x70,0x65,0x6e,0x64,0x43,0x68,0x69,0x6c,0x64,0x28,0x64,0x6f,0x63,0x75,0x6d,0x65,0x6e,0x74,0x2e,0x67,0x65,0x74,0x45,0x6c,0x65,0x6d,0x65,0x6e,0x74,0x42,0x79,0x49,0x64,0x28,0x64,0x61,0x74,0x61,0x29,0x29,0x3b,0x0a,0x7d,0x0a};inline auto webview_embed_file_index_js = []() -> bool { files.insert({"index.js", {270,embed_file_index_js}});return true; }();
}