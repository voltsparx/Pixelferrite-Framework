#include <string_view>

extern "C" const char* pixelferrite_module_id() {
    return "payloads/cross_platform/echo_payload";
}

extern "C" const char* pixelferrite_module_category() {
    return "payload";
}

extern "C" const char* pixelferrite_module_summary() {
    return "Scaffold module implementation placeholder.";
}
