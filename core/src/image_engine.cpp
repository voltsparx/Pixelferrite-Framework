#include <pixelferrite/image_engine.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <sstream>
#include <string_view>
#include <vector>

namespace pixelferrite {

namespace {

constexpr std::string_view kBinaryMarkerBegin = "\nPFFSIM_HEX_BEGIN\n";
constexpr std::string_view kBinaryMarkerEnd = "\nPFFSIM_HEX_END\n";
constexpr std::string_view kSvgMarkerPrefix = "<!-- PFFSIM_HEX:";
constexpr std::string_view kSvgMarkerSuffix = " -->";

enum class ImageFormat {
    unknown,
    jpeg,
    png,
    gif,
    svg
};

std::string lower_copy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

bool read_binary(const std::filesystem::path& path, std::vector<unsigned char>& out) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return false;
    }
    out.assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
    return true;
}

bool write_binary(const std::filesystem::path& path, const std::vector<unsigned char>& data) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        return false;
    }
    if (!data.empty()) {
        output.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    }
    return static_cast<bool>(output);
}

bool read_text(const std::filesystem::path& path, std::string& out) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return false;
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();
    out = buffer.str();
    return true;
}

bool write_text(const std::filesystem::path& path, const std::string& data) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        return false;
    }
    output << data;
    return static_cast<bool>(output);
}

ImageFormat format_from_extension(const std::filesystem::path& path) {
    const std::string ext = lower_copy(path.extension().string());
    if (ext == ".jpg" || ext == ".jpeg") {
        return ImageFormat::jpeg;
    }
    if (ext == ".png") {
        return ImageFormat::png;
    }
    if (ext == ".gif") {
        return ImageFormat::gif;
    }
    if (ext == ".svg") {
        return ImageFormat::svg;
    }
    return ImageFormat::unknown;
}

bool starts_with(const std::vector<unsigned char>& data, const std::initializer_list<unsigned char>& signature) {
    if (data.size() < signature.size()) {
        return false;
    }
    return std::equal(signature.begin(), signature.end(), data.begin());
}

ImageFormat detect_format(const std::filesystem::path& path, const std::vector<unsigned char>& data) {
    if (starts_with(data, {0xFF, 0xD8})) {
        return ImageFormat::jpeg;
    }
    if (starts_with(data, {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A})) {
        return ImageFormat::png;
    }
    if (data.size() >= 6) {
        const std::string header(reinterpret_cast<const char*>(data.data()), 6);
        if (header == "GIF87a" || header == "GIF89a") {
            return ImageFormat::gif;
        }
    }

    const ImageFormat ext_guess = format_from_extension(path);
    if (ext_guess == ImageFormat::svg) {
        std::string text(data.begin(), data.end());
        const std::string lowered = lower_copy(std::move(text));
        if (lowered.find("<svg") != std::string::npos) {
            return ImageFormat::svg;
        }
    }
    return ImageFormat::unknown;
}

std::string format_name(ImageFormat format) {
    switch (format) {
        case ImageFormat::jpeg:
            return "jpeg";
        case ImageFormat::png:
            return "png";
        case ImageFormat::gif:
            return "gif";
        case ImageFormat::svg:
            return "svg";
        default:
            return "unknown";
    }
}

std::string hex_encode(const std::string& input) {
    static constexpr std::array<char, 16> kHex = {
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
    };

    std::string out;
    out.reserve(input.size() * 2);
    for (unsigned char ch : input) {
        out.push_back(kHex[ch >> 4]);
        out.push_back(kHex[ch & 0x0F]);
    }
    return out;
}

bool hex_value(char ch, unsigned char& value) {
    if (ch >= '0' && ch <= '9') {
        value = static_cast<unsigned char>(ch - '0');
        return true;
    }
    if (ch >= 'a' && ch <= 'f') {
        value = static_cast<unsigned char>(ch - 'a' + 10);
        return true;
    }
    if (ch >= 'A' && ch <= 'F') {
        value = static_cast<unsigned char>(ch - 'A' + 10);
        return true;
    }
    return false;
}

bool hex_decode(const std::string& hex, std::string& out) {
    if (hex.size() % 2 != 0) {
        return false;
    }
    out.clear();
    out.reserve(hex.size() / 2);
    for (std::size_t i = 0; i < hex.size(); i += 2) {
        unsigned char hi = 0;
        unsigned char lo = 0;
        if (!hex_value(hex[i], hi) || !hex_value(hex[i + 1], lo)) {
            return false;
        }
        out.push_back(static_cast<char>((hi << 4) | lo));
    }
    return true;
}

std::size_t find_case_insensitive(const std::string& haystack, const std::string& needle) {
    const std::string lower_haystack = lower_copy(haystack);
    const std::string lower_needle = lower_copy(needle);
    return lower_haystack.rfind(lower_needle);
}

}  // namespace

const char* ImageEngine::name() const noexcept {
    return "image_engine";
}

bool ImageEngine::supports_format(const std::filesystem::path& image_path) const {
    std::vector<unsigned char> bytes;
    if (!read_binary(image_path, bytes) || bytes.empty()) {
        return false;
    }
    return detect_format(image_path, bytes) != ImageFormat::unknown;
}

std::string ImageEngine::supported_extensions() const {
    return ".jpg,.jpeg,.png,.gif,.svg";
}

ImageEmbedReport ImageEngine::embed_simulation_code(
    const std::filesystem::path& input_image,
    const std::filesystem::path& output_image,
    const std::string& simulation_code
) const {
    ImageEmbedReport report;
    std::vector<unsigned char> input_bytes;
    if (!read_binary(input_image, input_bytes) || input_bytes.empty()) {
        report.message = "unable to read input image";
        return report;
    }

    report.input_size = input_bytes.size();
    const ImageFormat format = detect_format(input_image, input_bytes);
    report.format = format_name(format);
    if (format == ImageFormat::unknown) {
        report.message = "unsupported or invalid image format";
        return report;
    }

    const std::string code_hex = hex_encode(simulation_code);
    if (format == ImageFormat::svg) {
        std::string svg_text;
        if (!read_text(input_image, svg_text)) {
            report.message = "unable to parse svg input";
            return report;
        }

        const std::string marker = std::string(kSvgMarkerPrefix) + code_hex + std::string(kSvgMarkerSuffix);
        const std::size_t close_pos = find_case_insensitive(svg_text, "</svg>");
        if (close_pos == std::string::npos) {
            svg_text += "\n" + marker + "\n";
        } else {
            svg_text.insert(close_pos, "\n" + marker + "\n");
        }

        if (!write_text(output_image, svg_text)) {
            report.message = "failed to write output svg";
            return report;
        }

        std::vector<unsigned char> output_bytes;
        if (!read_binary(output_image, output_bytes)) {
            report.message = "failed to verify output svg";
            return report;
        }

        report.output_size = output_bytes.size();
        report.integrity_ok = lower_copy(svg_text).find("<svg") != std::string::npos;
        report.success = report.integrity_ok;
        report.message = report.integrity_ok ? "simulation code embedded in svg comment" : "svg integrity check failed";
        return report;
    }

    std::vector<unsigned char> output_bytes = input_bytes;
    const std::string payload_block =
        std::string(kBinaryMarkerBegin) + code_hex + std::string(kBinaryMarkerEnd);
    output_bytes.insert(output_bytes.end(), payload_block.begin(), payload_block.end());

    if (!write_binary(output_image, output_bytes)) {
        report.message = "failed to write output image";
        return report;
    }

    std::vector<unsigned char> verify_bytes;
    if (!read_binary(output_image, verify_bytes)) {
        report.message = "failed to verify output image";
        return report;
    }

    report.output_size = verify_bytes.size();
    const bool prefix_unchanged =
        verify_bytes.size() >= input_bytes.size() &&
        std::equal(input_bytes.begin(), input_bytes.end(), verify_bytes.begin());
    const ImageFormat verify_format = detect_format(output_image, verify_bytes);
    report.integrity_ok = prefix_unchanged && verify_format == format;
    report.success = report.integrity_ok;
    report.message = report.integrity_ok ? "simulation code embedded safely" : "integrity verification failed";
    return report;
}

ImageExtractReport ImageEngine::extract_simulation_code(const std::filesystem::path& image_path) const {
    ImageExtractReport report;
    std::vector<unsigned char> bytes;
    if (!read_binary(image_path, bytes) || bytes.empty()) {
        report.message = "unable to read image";
        return report;
    }

    const ImageFormat format = detect_format(image_path, bytes);
    report.format = format_name(format);
    if (format == ImageFormat::unknown) {
        report.message = "unsupported or invalid image format";
        return report;
    }

    std::string encoded;
    if (format == ImageFormat::svg) {
        std::string text;
        if (!read_text(image_path, text)) {
            report.message = "unable to parse svg";
            return report;
        }

        const std::size_t start = text.rfind(std::string(kSvgMarkerPrefix));
        if (start == std::string::npos) {
            report.message = "no simulation marker found";
            return report;
        }
        const std::size_t hex_start = start + kSvgMarkerPrefix.size();
        const std::size_t end = text.find(std::string(kSvgMarkerSuffix), hex_start);
        if (end == std::string::npos || end <= hex_start) {
            report.message = "invalid svg marker";
            return report;
        }
        encoded = text.substr(hex_start, end - hex_start);
    } else {
        const std::string data(bytes.begin(), bytes.end());
        const std::size_t start = data.rfind(std::string(kBinaryMarkerBegin));
        if (start == std::string::npos) {
            report.message = "no simulation marker found";
            return report;
        }
        const std::size_t hex_start = start + kBinaryMarkerBegin.size();
        const std::size_t end = data.find(std::string(kBinaryMarkerEnd), hex_start);
        if (end == std::string::npos || end <= hex_start) {
            report.message = "invalid embedded marker";
            return report;
        }
        encoded = data.substr(hex_start, end - hex_start);
    }

    std::string decoded;
    if (!hex_decode(encoded, decoded)) {
        report.message = "marker present but content is corrupted";
        return report;
    }

    report.success = true;
    report.simulation_code = decoded;
    report.message = "simulation code extracted";
    return report;
}

}  // namespace pixelferrite
