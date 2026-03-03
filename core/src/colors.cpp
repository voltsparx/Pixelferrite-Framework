#include <pixelferrite/colors.hpp>

#include <cstdio>
#include <cstdlib>
#include <string>

#if defined(_WIN32)
#include <io.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace pixelferrite::colors {

namespace {

bool g_initialized = false;
bool g_enabled = false;

bool is_truthy(const char* value) {
    if (value == nullptr) {
        return false;
    }

    const std::string raw(value);
    return raw == "1" || raw == "true" || raw == "TRUE" || raw == "yes" || raw == "on";
}

bool stream_is_tty() {
#if defined(_WIN32)
    return _isatty(_fileno(stdout)) != 0;
#else
    return isatty(fileno(stdout)) != 0;
#endif
}

void enable_windows_vt_mode() {
#if defined(_WIN32)
    const HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    if (output == INVALID_HANDLE_VALUE || output == nullptr) {
        return;
    }

    DWORD mode = 0;
    if (!GetConsoleMode(output, &mode)) {
        return;
    }

    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(output, mode);
#endif
}

}  // namespace

void initialize() {
    if (g_initialized) {
        return;
    }
    g_initialized = true;

    if (std::getenv("NO_COLOR") != nullptr) {
        g_enabled = false;
        return;
    }

    if (is_truthy(std::getenv("PF_NO_COLOR"))) {
        g_enabled = false;
        return;
    }

    if (is_truthy(std::getenv("PF_COLOR_ALWAYS"))) {
        g_enabled = true;
        enable_windows_vt_mode();
        return;
    }

    g_enabled = stream_is_tty();
    if (g_enabled) {
        enable_windows_vt_mode();
    }
}

bool enabled() {
    if (!g_initialized) {
        initialize();
    }
    return g_enabled;
}

std::string wrap(std::string_view text, std::string_view ansi_code) {
    if (!enabled() || ansi_code.empty()) {
        return std::string(text);
    }

    std::string out;
    out.reserve(ansi_code.size() + text.size() + reset().size());
    out.append(ansi_code);
    out.append(text);
    out.append(reset());
    return out;
}

std::string_view reset() {
    return "\033[0m";
}

std::string_view dim() {
    return "\033[2m";
}

std::string_view bold() {
    return "\033[1m";
}

std::string_view brand() {
    return "\033[38;5;46m";
}

std::string_view accent() {
    return "\033[38;5;118m";
}

std::string_view orange() {
    return "\033[38;5;214m";
}

std::string_view gray() {
    return "\033[38;5;245m";
}

std::string_view info() {
    return "\033[38;5;84m";
}

std::string_view success() {
    return "\033[38;5;47m";
}

std::string_view warning() {
    return "\033[38;5;214m";
}

std::string_view error() {
    return "\033[38;5;203m";
}

std::string_view prompt() {
    return "\033[38;5;46m";
}

std::string_view payload() {
    return "\033[38;5;83m";
}

std::string_view exploit() {
    return "\033[38;5;120m";
}

std::string_view auxiliary() {
    return "\033[38;5;71m";
}

std::string_view encoder() {
    return "\033[38;5;107m";
}

std::string_view evasion() {
    return "\033[38;5;78m";
}

std::string_view nop() {
    return "\033[38;5;41m";
}

std::string_view transport() {
    return "\033[38;5;49m";
}

std::string_view detection() {
    return "\033[38;5;119m";
}

std::string_view analysis() {
    return "\033[38;5;113m";
}

std::string_view lab() {
    return "\033[38;5;77m";
}

std::string_view for_category(std::string_view category) {
    if (category == "payload") {
        return payload();
    }
    if (category == "exploit") {
        return exploit();
    }
    if (category == "auxiliary") {
        return auxiliary();
    }
    if (category == "encoder") {
        return encoder();
    }
    if (category == "evasion") {
        return evasion();
    }
    if (category == "nop") {
        return nop();
    }
    if (category == "transport") {
        return transport();
    }
    if (category == "detection") {
        return detection();
    }
    if (category == "analysis") {
        return analysis();
    }
    if (category == "lab") {
        return lab();
    }
    return info();
}

}  // namespace pixelferrite::colors
