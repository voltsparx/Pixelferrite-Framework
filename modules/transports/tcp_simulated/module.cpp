#include <string_view>

extern "C" const char* pixelferrite_module_id() {
    return "transports/tcp_simulated";
}

extern "C" const char* pixelferrite_module_category() {
    return "transport";
}

extern "C" const char* pixelferrite_module_summary() {
    return "Safe transport simulation profile for TCP-like channel modeling.";
}

