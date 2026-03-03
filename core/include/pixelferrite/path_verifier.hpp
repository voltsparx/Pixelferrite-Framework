#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace pixelferrite::path_verifier {

struct CheckResult {
    bool ok = false;
    std::filesystem::path path;
    std::string detail;
};

std::string display_path(const std::filesystem::path& path);

CheckResult require_existing_directory(
    const std::filesystem::path& path,
    std::string_view label
);

CheckResult require_existing_file(
    const std::filesystem::path& path,
    std::string_view label
);

CheckResult ensure_directory(
    const std::filesystem::path& path,
    std::string_view label
);

CheckResult require_writable_file_path(
    const std::filesystem::path& path,
    std::string_view label,
    bool create_parent_dirs = true
);

}  // namespace pixelferrite::path_verifier

