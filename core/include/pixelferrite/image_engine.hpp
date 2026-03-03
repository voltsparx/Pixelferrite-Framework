#pragma once

#include <filesystem>
#include <string>

namespace pixelferrite {

struct ImageEmbedReport {
    bool success = false;
    bool integrity_ok = false;
    std::string format;
    std::string message;
    std::size_t input_size = 0;
    std::size_t output_size = 0;
};

struct ImageExtractReport {
    bool success = false;
    std::string format;
    std::string message;
    std::string simulation_code;
};

class ImageEngine {
public:
    const char* name() const noexcept;
    bool supports_format(const std::filesystem::path& image_path) const;
    std::string supported_extensions() const;

    ImageEmbedReport embed_simulation_code(
        const std::filesystem::path& input_image,
        const std::filesystem::path& output_image,
        const std::string& simulation_code
    ) const;

    ImageExtractReport extract_simulation_code(
        const std::filesystem::path& image_path
    ) const;
};

}  // namespace pixelferrite
