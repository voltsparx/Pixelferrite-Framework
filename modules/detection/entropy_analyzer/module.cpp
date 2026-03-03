#include <string_view>

extern "C" const char* pixelferrite_module_id() {
    return "detection/entropy_analyzer";
}

extern "C" const char* pixelferrite_module_category() {
    return "detection";
}

extern "C" const char* pixelferrite_module_summary() {
    return "Scaffold module implementation placeholder.";
}
