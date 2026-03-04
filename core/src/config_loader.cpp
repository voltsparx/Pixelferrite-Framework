#include <pixelferrite/config_loader.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <unistd.h>
#else
#include <unistd.h>
#endif

namespace pixelferrite {

namespace {

std::string trim_copy(std::string value) {
    const auto is_not_space = [](unsigned char ch) { return !std::isspace(ch); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), is_not_space));
    value.erase(std::find_if(value.rbegin(), value.rend(), is_not_space).base(), value.end());
    return value;
}

std::string strip_quotes(std::string value) {
    value = trim_copy(std::move(value));
    if (value.size() >= 2) {
        const char first = value.front();
        const char last = value.back();
        if ((first == '"' && last == '"') || (first == '\'' && last == '\'')) {
            value = value.substr(1, value.size() - 2);
        }
    }
    return value;
}

std::string strip_inline_comment(const std::string& value) {
    bool in_single = false;
    bool in_double = false;
    for (std::size_t i = 0; i < value.size(); ++i) {
        const char ch = value[i];
        if (ch == '\'' && !in_double) {
            in_single = !in_single;
            continue;
        }
        if (ch == '"' && !in_single) {
            in_double = !in_double;
            continue;
        }
        if (ch == '#' && !in_single && !in_double) {
            if (i == 0 || std::isspace(static_cast<unsigned char>(value[i - 1]))) {
                return value.substr(0, i);
            }
        }
    }
    return value;
}

std::size_t indentation_of(const std::string& line) {
    std::size_t count = 0;
    for (char ch : line) {
        if (ch == ' ') {
            ++count;
            continue;
        }
        if (ch == '\t') {
            count += 2;
            continue;
        }
        break;
    }
    return count;
}

bool parse_key_value(const std::string& line, std::string& key, std::string& value) {
    const std::size_t sep = line.find(':');
    if (sep == std::string::npos) {
        return false;
    }

    key = trim_copy(line.substr(0, sep));
    value = trim_copy(strip_inline_comment(line.substr(sep + 1)));
    value = strip_quotes(value);
    return !key.empty();
}

std::filesystem::path executable_path() {
#if defined(_WIN32)
    std::array<char, MAX_PATH> buffer{};
    const DWORD length = GetModuleFileNameA(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (length == 0 || length >= buffer.size()) {
        return {};
    }
    return std::filesystem::path(std::string(buffer.data(), length));
#elif defined(__APPLE__)
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    if (size == 0) {
        return {};
    }
    std::string buffer(size, '\0');
    if (_NSGetExecutablePath(buffer.data(), &size) != 0) {
        return {};
    }
    return std::filesystem::path(buffer.c_str());
#else
    std::array<char, 4096> buffer{};
    const ssize_t length = readlink("/proc/self/exe", buffer.data(), buffer.size() - 1);
    if (length <= 0) {
        return {};
    }
    buffer[static_cast<std::size_t>(length)] = '\0';
    return std::filesystem::path(buffer.data());
#endif
}

std::filesystem::path resolve_relative_to(const std::filesystem::path& base, const std::filesystem::path& value) {
    if (value.is_absolute() || base.empty()) {
        return value;
    }
    return base / value;
}

void parse_framework_file(const std::filesystem::path& path, RuntimeConfig& config) {
    std::ifstream input(path);
    if (!input) {
        config.warnings.push_back("missing " + path.filename().string() + "; defaults applied");
        return;
    }

    bool in_framework = false;
    std::string line;
    while (std::getline(input, line)) {
        const std::string stripped = trim_copy(strip_inline_comment(line));
        if (stripped.empty()) {
            continue;
        }

        if (indentation_of(line) == 0) {
            in_framework = (stripped == "framework:");
            continue;
        }
        if (!in_framework) {
            continue;
        }

        std::string key;
        std::string value;
        if (!parse_key_value(stripped, key, value)) {
            continue;
        }

        if (key == "name") {
            config.framework.name = value;
        } else if (key == "version") {
            config.framework.version = value;
        } else if (key == "modules_root") {
            config.framework.modules_root = value;
        } else if (key == "plugins_root") {
            config.framework.plugins_root = value;
        } else if (key == "data_root") {
            config.framework.data_root = value;
        } else if (key == "safety_mode") {
            config.framework.safety_mode = value;
        }
    }
}

void parse_logging_file(const std::filesystem::path& path, RuntimeConfig& config) {
    std::ifstream input(path);
    if (!input) {
        config.warnings.push_back("missing " + path.filename().string() + "; defaults applied");
        return;
    }

    bool in_logging = false;
    bool in_sinks = false;
    std::string line;
    while (std::getline(input, line)) {
        const std::string stripped = trim_copy(strip_inline_comment(line));
        if (stripped.empty()) {
            continue;
        }

        const std::size_t indent = indentation_of(line);
        if (indent == 0) {
            in_logging = (stripped == "logging:");
            in_sinks = false;
            continue;
        }
        if (!in_logging) {
            continue;
        }

        if (stripped == "sinks:") {
            in_sinks = true;
            continue;
        }

        if (in_sinks && stripped.rfind("- ", 0) == 0) {
            std::string item = trim_copy(stripped.substr(2));
            std::string item_key;
            std::string item_value;
            if (parse_key_value(item, item_key, item_value)) {
                if (item_key == "file" && !item_value.empty()) {
                    config.logging.sinks.push_back(item_value);
                }
            } else if (!item.empty()) {
                config.logging.sinks.push_back(strip_quotes(item));
            }
            continue;
        }

        std::string key;
        std::string value;
        if (parse_key_value(stripped, key, value)) {
            if (key == "level") {
                config.logging.level = value;
            } else if (in_sinks && key == "file" && !value.empty()) {
                config.logging.sinks.push_back(value);
            }
            continue;
        }
    }
}

void parse_workspace_file(const std::filesystem::path& path, RuntimeConfig& config) {
    std::ifstream input(path);
    if (!input) {
        config.warnings.push_back("missing " + path.filename().string() + "; defaults applied");
        return;
    }

    bool in_workspace = false;
    std::string line;
    while (std::getline(input, line)) {
        const std::string stripped = trim_copy(strip_inline_comment(line));
        if (stripped.empty()) {
            continue;
        }

        if (indentation_of(line) == 0) {
            in_workspace = (stripped == "workspace:");
            continue;
        }
        if (!in_workspace) {
            continue;
        }

        std::string key;
        std::string value;
        if (!parse_key_value(stripped, key, value)) {
            continue;
        }

        if (key == "default") {
            config.workspace.default_workspace = value;
        } else if (key == "active") {
            config.workspace.active_workspace = value;
        } else if (key == "temp_root") {
            config.workspace.temp_root = value;
        } else if (key == "reports_root") {
            config.workspace.reports_root = value;
        } else if (key == "sessions_root") {
            config.workspace.sessions_root = value;
        }
    }
}

}  // namespace

std::filesystem::path ConfigLoader::resolve_config_dir(const std::filesystem::path& preferred_config_dir) {
    auto valid_config_dir = [](const std::filesystem::path& path) {
        return !path.empty() && std::filesystem::exists(path) && std::filesystem::is_directory(path);
    };

    if (valid_config_dir(preferred_config_dir)) {
        return preferred_config_dir;
    }

    if (const char* env_config = std::getenv("PF_CONFIG_DIR"); env_config != nullptr && *env_config != '\0') {
        const std::filesystem::path env_path(env_config);
        if (valid_config_dir(env_path)) {
            return env_path;
        }
    }

    std::vector<std::filesystem::path> candidates;
    candidates.push_back(std::filesystem::current_path() / "config");

    if (const char* env_home = std::getenv("PF_HOME"); env_home != nullptr && *env_home != '\0') {
        candidates.push_back(std::filesystem::path(env_home) / "config");
    }

    const std::filesystem::path exe = executable_path();
    if (!exe.empty()) {
        const std::filesystem::path exe_dir = exe.parent_path();
        candidates.push_back(exe_dir / "config");
        candidates.push_back(exe_dir.parent_path() / "config");
        candidates.push_back(exe_dir.parent_path().parent_path() / "config");
    }

    for (const std::filesystem::path& candidate : candidates) {
        if (valid_config_dir(candidate)) {
            return candidate;
        }
    }

    return {};
}

RuntimeConfig ConfigLoader::load(const std::filesystem::path& preferred_config_dir) {
    RuntimeConfig config;
    config.config_dir = resolve_config_dir(preferred_config_dir);
    if (config.config_dir.empty()) {
        config.warnings.push_back("config directory not found; using built-in defaults");
        return config;
    }

    parse_framework_file(config.config_dir / "framework.yml", config);
    parse_logging_file(config.config_dir / "logging.yml", config);
    parse_workspace_file(config.config_dir / "workspace.yml", config);

    if (config.framework.modules_root.empty()) {
        config.framework.modules_root = "modules";
        config.warnings.push_back("framework.modules_root was empty; reverted to default");
    }
    if (config.workspace.default_workspace.empty()) {
        config.workspace.default_workspace = "default";
        config.warnings.push_back("workspace.default was empty; reverted to default");
    }
    if (config.workspace.active_workspace.empty()) {
        config.workspace.active_workspace = config.workspace.default_workspace;
    }
    if (config.framework.safety_mode.empty()) {
        config.framework.safety_mode = "simulation_only";
    }

    // Normalize config_dir if possible for clearer diagnostics.
    std::error_code ec;
    const std::filesystem::path canonical = std::filesystem::weakly_canonical(config.config_dir, ec);
    if (!ec && !canonical.empty()) {
        config.config_dir = canonical;
    }

    // Resolve known relative paths against the config parent.
    const std::filesystem::path config_parent = config.config_dir.parent_path();
    config.framework.modules_root = resolve_relative_to(config_parent, config.framework.modules_root).generic_string();
    config.framework.plugins_root = resolve_relative_to(config_parent, config.framework.plugins_root).generic_string();
    config.framework.data_root = resolve_relative_to(config_parent, config.framework.data_root).generic_string();

    return config;
}

}  // namespace pixelferrite
