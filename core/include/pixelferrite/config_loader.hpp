#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace pixelferrite {

struct FrameworkConfig {
    std::string name = "PixelFerrite";
    std::string version = "0.1.0";
    std::string modules_root = "modules";
    std::string plugins_root = "plugins";
    std::string data_root = "data";
    std::string safety_mode = "simulation_only";
};

struct LoggingConfig {
    std::string level = "info";
    std::vector<std::string> sinks;
};

struct WorkspaceConfig {
    std::string default_workspace = "default";
    std::string active_workspace = "default";
    std::string temp_root = "tmp";
    std::string reports_root = "var/reports";
    std::string sessions_root = "var/sessions";
};

struct RuntimeConfig {
    FrameworkConfig framework;
    LoggingConfig logging;
    WorkspaceConfig workspace;
    std::filesystem::path config_dir;
    std::vector<std::string> warnings;
};

class ConfigLoader {
public:
    static RuntimeConfig load(const std::filesystem::path& preferred_config_dir = {});
    static std::filesystem::path resolve_config_dir(const std::filesystem::path& preferred_config_dir = {});
};

}  // namespace pixelferrite

