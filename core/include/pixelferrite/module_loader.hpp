#pragma once

#include <string>
#include <vector>

namespace pixelferrite {

struct ModuleOption {
    std::string name;
    bool required = false;
    std::string default_value;
    std::string description;
};

struct ModuleDescriptor {
    std::string id;
    std::string name;
    std::string category;
    std::string path;
    std::string author;
    std::string version;
    std::string safety_level;
    std::vector<std::string> supported_platforms;
    std::vector<std::string> supported_arch;
    std::vector<ModuleOption> options;
};

class ModuleLoader {
public:
    std::vector<ModuleDescriptor> discover(const std::string& modules_root) const;
    ModuleDescriptor find_by_id(
        const std::vector<ModuleDescriptor>& modules,
        const std::string& module_id
    ) const;
};

}  // namespace pixelferrite
