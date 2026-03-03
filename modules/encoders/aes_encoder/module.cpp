#include <string_view>

extern "C" const char* pixelferrite_module_id() {
    return "encoders/aes_encoder";
}

extern "C" const char* pixelferrite_module_category() {
    return "encoder";
}

extern "C" const char* pixelferrite_module_summary() {
    return "Scaffold module implementation placeholder.";
}
