#include <pixelferrite/path_verifier.hpp>

#include <chrono>
#include <fstream>
#include <sstream>
#include <system_error>

namespace pixelferrite::path_verifier {

namespace {

CheckResult make_result(
    bool ok,
    std::string_view label,
    const std::filesystem::path& path,
    const std::string& detail
) {
    CheckResult result;
    result.ok = ok;
    result.path = path;
    if (ok) {
        result.detail = std::string(label) + " ok: " + display_path(path);
    } else {
        result.detail = std::string(label) + " invalid: " + display_path(path) + " (" + detail + ")";
    }
    return result;
}

std::filesystem::path normalized_path(const std::filesystem::path& path) {
    std::error_code ec;
    const std::filesystem::path normalized = std::filesystem::weakly_canonical(path, ec);
    if (ec) {
        return path.lexically_normal();
    }
    return normalized;
}

}  // namespace

std::string display_path(const std::filesystem::path& path) {
    return normalized_path(path).generic_string();
}

CheckResult require_existing_directory(
    const std::filesystem::path& path,
    std::string_view label
) {
    std::error_code ec;
    if (path.empty()) {
        return make_result(false, label, path, "empty path");
    }
    if (!std::filesystem::exists(path, ec) || ec) {
        return make_result(false, label, path, "path does not exist");
    }
    if (!std::filesystem::is_directory(path, ec) || ec) {
        return make_result(false, label, path, "expected directory");
    }
    return make_result(true, label, path, {});
}

CheckResult require_existing_file(
    const std::filesystem::path& path,
    std::string_view label
) {
    std::error_code ec;
    if (path.empty()) {
        return make_result(false, label, path, "empty path");
    }
    if (!std::filesystem::exists(path, ec) || ec) {
        return make_result(false, label, path, "path does not exist");
    }
    if (!std::filesystem::is_regular_file(path, ec) || ec) {
        return make_result(false, label, path, "expected file");
    }
    return make_result(true, label, path, {});
}

CheckResult ensure_directory(
    const std::filesystem::path& path,
    std::string_view label
) {
    std::error_code ec;
    if (path.empty()) {
        return make_result(false, label, path, "empty path");
    }
    std::filesystem::create_directories(path, ec);
    if (ec) {
        return make_result(false, label, path, ec.message());
    }
    if (!std::filesystem::exists(path, ec) || ec) {
        return make_result(false, label, path, "path does not exist after create");
    }
    if (!std::filesystem::is_directory(path, ec) || ec) {
        return make_result(false, label, path, "expected directory");
    }
    return make_result(true, label, path, {});
}

CheckResult require_writable_file_path(
    const std::filesystem::path& path,
    std::string_view label,
    bool create_parent_dirs
) {
    if (path.empty()) {
        return make_result(false, label, path, "empty path");
    }

    std::error_code ec;
    if (std::filesystem::exists(path, ec) && std::filesystem::is_directory(path, ec)) {
        return make_result(false, label, path, "path is a directory");
    }

    std::filesystem::path parent = path.parent_path();
    if (parent.empty()) {
        parent = std::filesystem::current_path();
    }

    if (create_parent_dirs) {
        std::filesystem::create_directories(parent, ec);
        if (ec) {
            return make_result(false, label, path, ec.message());
        }
    }

    if (!std::filesystem::exists(parent, ec) || ec) {
        return make_result(false, label, path, "parent directory does not exist");
    }
    if (!std::filesystem::is_directory(parent, ec) || ec) {
        return make_result(false, label, path, "parent path is not a directory");
    }

    const auto tick = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const std::filesystem::path probe_file = parent / (".pff_write_probe_" + std::to_string(tick) + ".tmp");
    std::ofstream probe(probe_file, std::ios::trunc);
    if (!probe) {
        return make_result(false, label, path, "parent directory is not writable");
    }
    probe << "probe";
    probe.close();
    std::filesystem::remove(probe_file, ec);

    return make_result(true, label, path, {});
}

}  // namespace pixelferrite::path_verifier

