#include <pixelferrite/module_loader.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>
#include <system_error>
#include <utility>

namespace pixelferrite {

namespace {

std::string read_text_file(const std::filesystem::path& file_path) {
    std::ifstream input(file_path, std::ios::in | std::ios::binary);
    if (!input) {
        return {};
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

std::string extract_string_field(const std::string& json_text, const std::string& key) {
    const std::regex pattern("\"" + key + "\"\\s*:\\s*\"([^\"]*)\"");
    std::smatch match;
    if (!std::regex_search(json_text, match, pattern) || match.size() < 2) {
        return {};
    }
    return match[1].str();
}

std::vector<std::string> extract_string_array_field(const std::string& json_text, const std::string& key) {
    const std::regex outer_pattern("\"" + key + "\"\\s*:\\s*\\[([\\s\\S]*?)\\]");
    std::smatch outer_match;
    if (!std::regex_search(json_text, outer_match, outer_pattern) || outer_match.size() < 2) {
        return {};
    }

    const std::string body = outer_match[1].str();
    const std::regex item_pattern("\"([^\"]+)\"");
    std::vector<std::string> items;

    for (
        std::sregex_iterator iter(body.begin(), body.end(), item_pattern), end;
        iter != end;
        ++iter
    ) {
        items.push_back((*iter)[1].str());
    }

    return items;
}

bool extract_bool_field(const std::string& json_text, const std::string& key, bool default_value = false) {
    const std::regex pattern("\"" + key + "\"\\s*:\\s*(true|false)");
    std::smatch match;
    if (!std::regex_search(json_text, match, pattern) || match.size() < 2) {
        return default_value;
    }
    return match[1].str() == "true";
}

std::vector<ModuleOption> extract_options_field(const std::string& json_text) {
    const std::regex outer_pattern("\"options\"\\s*:\\s*\\[([\\s\\S]*?)\\]");
    std::smatch outer_match;
    if (!std::regex_search(json_text, outer_match, outer_pattern) || outer_match.size() < 2) {
        return {};
    }

    const std::string body = outer_match[1].str();
    const std::regex object_pattern("\\{([\\s\\S]*?)\\}");
    std::vector<ModuleOption> options;

    for (std::sregex_iterator it(body.begin(), body.end(), object_pattern), end; it != end; ++it) {
        const std::string object_body = (*it)[1].str();
        ModuleOption option;
        option.name = extract_string_field(object_body, "name");
        option.required = extract_bool_field(object_body, "required", false);
        option.default_value = extract_string_field(object_body, "default");
        option.description = extract_string_field(object_body, "description");

        if (!option.name.empty()) {
            options.push_back(std::move(option));
        }
    }

    return options;
}

}  // namespace

std::vector<ModuleDescriptor> ModuleLoader::discover(const std::string& modules_root) const {
    std::vector<ModuleDescriptor> modules;

    const std::filesystem::path root(modules_root);
    if (!std::filesystem::exists(root)) {
        return modules;
    }

    std::error_code ec;
    std::filesystem::recursive_directory_iterator it(root, ec);
    std::filesystem::recursive_directory_iterator end;
    if (ec) {
        return modules;
    }

    for (; it != end; it.increment(ec)) {
        if (ec) {
            break;
        }

        if (!it->is_regular_file()) {
            continue;
        }

        if (it->path().filename() != "module.json") {
            continue;
        }

        const std::filesystem::path module_dir = it->path().parent_path();
        const std::filesystem::path relative_dir = std::filesystem::relative(module_dir, root, ec);
        if (ec) {
            continue;
        }

        ModuleDescriptor descriptor;
        descriptor.id = relative_dir.generic_string();
        descriptor.name = descriptor.id;
        descriptor.path = module_dir.generic_string();

        auto segment = relative_dir.begin();
        descriptor.category = (segment != relative_dir.end()) ? segment->string() : "unknown";

        const std::string manifest = read_text_file(it->path());
        if (!manifest.empty()) {
            const std::string id_value = extract_string_field(manifest, "id");
            if (!id_value.empty()) {
                descriptor.id = id_value;
            }

            const std::string name_value = extract_string_field(manifest, "name");
            if (!name_value.empty()) {
                descriptor.name = name_value;
            }

            const std::string category_value = extract_string_field(manifest, "category");
            if (!category_value.empty()) {
                descriptor.category = category_value;
            }

            descriptor.author = extract_string_field(manifest, "author");
            descriptor.version = extract_string_field(manifest, "version");
            descriptor.safety_level = extract_string_field(manifest, "safety_level");
            descriptor.supported_platforms = extract_string_array_field(manifest, "supported_platforms");
            descriptor.supported_arch = extract_string_array_field(manifest, "supported_arch");
            descriptor.options = extract_options_field(manifest);
        }

        modules.push_back(std::move(descriptor));
    }

    std::sort(modules.begin(), modules.end(), [](const ModuleDescriptor& lhs, const ModuleDescriptor& rhs) {
        return lhs.id < rhs.id;
    });

    return modules;
}

ModuleDescriptor ModuleLoader::find_by_id(
    const std::vector<ModuleDescriptor>& modules,
    const std::string& module_id
) const {
    const auto found = std::find_if(modules.begin(), modules.end(), [&](const ModuleDescriptor& module) {
        return module.id == module_id;
    });

    return (found != modules.end()) ? *found : ModuleDescriptor{};
}

}  // namespace pixelferrite
