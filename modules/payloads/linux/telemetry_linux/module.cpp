#include <string_view>

extern "C" const char* pixelferrite_module_id() {
    return "payloads/linux/telemetry_linux";
}

extern "C" const char* pixelferrite_module_category() {
    return "payload";
}

extern "C" const char* pixelferrite_module_summary() {
    return "Scaffold module implementation placeholder.";
}
