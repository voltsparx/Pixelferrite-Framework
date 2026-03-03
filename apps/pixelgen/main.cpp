#include <pixelferrite/module_loader.hpp>
#include <pixelferrite/colors.hpp>
#include <pixelferrite/image_engine.hpp>
#include <pixelferrite/path_verifier.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <unistd.h>
#else
#include <unistd.h>
#endif

namespace {

struct CliOptions {
    bool help_requested = false;
    std::string list_target;
    std::string payload;
    std::string encoder;
    std::string format = "json";
    std::string output;
    std::string input_image;
    std::string image_output;
    std::string output_name;
    std::string output_dir;
    std::string transport = "http";
    std::string lhost;
    std::string rhost;
    int lport = 0;
    std::string platform = "cross_platform";
    int iterations = 1;
};

void print_help(std::string_view exe_name) {
    std::cout
        << pixelferrite::colors::wrap("pixelgen", pixelferrite::colors::brand())
        << " " << pixelferrite::colors::wrap("(image-only simulator)", pixelferrite::colors::dim())
        << "\n\n"
        << pixelferrite::colors::wrap("Usage:", pixelferrite::colors::accent()) << '\n'
        << "  " << exe_name << " -l <payloads|exploits|auxiliary|encoders|evasion|nops|transports|detection|analysis|lab|formats|all>\n"
        << "  " << exe_name << " -p <payload> [-e <encoder>] [-f <format>] [-o <file>] [--carrier <image>] [--image-out <file>]\n\n"
        << pixelferrite::colors::wrap("Options:", pixelferrite::colors::accent()) << '\n'
        << "  -l, --list <target>     List payloads/encoders/formats/all\n"
        << "  -p, --payload <id>      Select payload module id\n"
        << "  -e, --encoder <id>      Select encoder module id\n"
        << "  -t, --transport <type>  Transport profile: tcp|http|https|dns|quic|tls|file\n"
        << "  -f, --format <format>   Output format: json|txt|yaml (default json)\n"
        << "  -o, --out <file>        Output artifact path\n"
        << "      --input <image>     Input carrier image path (alias: --carrier)\n"
        << "      --image-out <file>  Output carrier path for embedded simulation metadata\n"
        << "      --name <filename>   Output image file name (used with --carrier)\n"
        << "      --dir <directory>   Output image directory (default current directory)\n"
        << "      --lhost <addr>      Simulated local host value for metadata\n"
        << "      --lport <port>      Simulated local port value for metadata\n"
        << "      --rhost <addr>      Simulated remote target value for metadata\n"
        << "      --platform <name>   Platform hint for filtering (default cross_platform)\n"
        << "      --iterations <n>    Iteration count for mutation simulation (default 1)\n"
        << "  -h, --help              Show this help\n\n"
        << pixelferrite::colors::wrap("Examples:", pixelferrite::colors::accent()) << '\n'
        << "  " << exe_name << " -l payloads\n"
        << "  " << exe_name << " -p payloads/windows/telemetry_win -t https -e encoders/aes_encoder -f json -o pixelgen_out.json\n"
        << "  " << exe_name << " -p payloads/windows/telemetry_win -t tcp --carrier data/carriers/sample.png --name vm_test_payload.png\n"
        << "  " << exe_name << " -p payloads/linux/telemetry_linux --carrier data/carriers/sample.jpg --dir ./out --name linux_sim.jpg --rhost 192.168.56.101 --lhost 192.168.56.1 --lport 4444\n";
}

std::string now_string() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t time = std::chrono::system_clock::to_time_t(now);

    std::tm local_time{};
#if defined(_WIN32)
    localtime_s(&local_time, &time);
#else
    localtime_r(&time, &local_time);
#endif

    std::ostringstream stream;
    stream << std::put_time(&local_time, "%Y-%m-%d %H:%M:%S");
    return stream.str();
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

std::filesystem::path home_dir() {
#if defined(_WIN32)
    if (const char* profile = std::getenv("USERPROFILE"); profile != nullptr && *profile != '\0') {
        return std::filesystem::path(profile);
    }
    if (const char* appdata = std::getenv("APPDATA"); appdata != nullptr && *appdata != '\0') {
        return std::filesystem::path(appdata).parent_path();
    }
#else
    if (const char* home = std::getenv("HOME"); home != nullptr && *home != '\0') {
        return std::filesystem::path(home);
    }
#endif
    return std::filesystem::current_path();
}

std::filesystem::path runtime_data_root() {
    if (const char* custom = std::getenv("PF_DATA_HOME"); custom != nullptr && *custom != '\0') {
        return std::filesystem::path(custom);
    }
#if defined(_WIN32)
    if (const char* appdata = std::getenv("APPDATA"); appdata != nullptr && *appdata != '\0') {
        return std::filesystem::path(appdata) / "PixelFerrite";
    }
    return home_dir() / "AppData" / "Roaming" / "PixelFerrite";
#elif defined(__APPLE__)
    return home_dir() / "Library" / "Application Support" / "pixelferrite";
#else
    return home_dir() / ".local" / "share" / "pixelferrite";
#endif
}

std::filesystem::path default_artifact_path() {
    return runtime_data_root() / "workspace" / "default" / "tmp" / "pixelgen_artifact.json";
}

std::string compact_timestamp() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t time = std::chrono::system_clock::to_time_t(now);

    std::tm local_time{};
#if defined(_WIN32)
    localtime_s(&local_time, &time);
#else
    localtime_r(&time, &local_time);
#endif

    std::ostringstream stream;
    stream << std::put_time(&local_time, "%Y%m%d_%H%M%S");
    return stream.str();
}

std::string payload_leaf(std::string payload) {
    const std::size_t pos = payload.find_last_of('/');
    if (pos == std::string::npos) {
        return payload;
    }
    return payload.substr(pos + 1);
}

std::string normalize_transport(std::string raw) {
    std::transform(raw.begin(), raw.end(), raw.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });

    if (raw == "https") {
        return "https";
    }
    if (raw == "tls") {
        return "tls";
    }
    if (raw == "tcp") {
        return "tcp";
    }
    if (raw == "http") {
        return "http";
    }
    if (raw == "dns") {
        return "dns";
    }
    if (raw == "quic") {
        return "quic";
    }
    if (raw == "file") {
        return "file";
    }
    return {};
}

std::string transport_module_id(const std::string& transport) {
    if (transport == "https" || transport == "tls") {
        return "transports/tls_dualstack_simulated";
    }
    if (transport == "dns") {
        return "transports/dns_simulated";
    }
    if (transport == "quic") {
        return "transports/quic_simulated";
    }
    if (transport == "file") {
        return "transports/file_drop_simulated";
    }
    if (transport == "http") {
        return "transports/http_simulated";
    }
    if (transport == "tcp") {
        return "transports/tcp_simulated";
    }
    return "transports/http_simulated";
}

std::filesystem::path resolve_image_output_path(const CliOptions& options) {
    if (options.input_image.empty()) {
        return {};
    }
    if (!options.image_output.empty()) {
        return std::filesystem::path(options.image_output);
    }

    const std::filesystem::path carrier(options.input_image);
    std::string extension = carrier.extension().string();
    if (extension.empty()) {
        extension = ".png";
    }

    std::string file_name = options.output_name;
    if (file_name.empty()) {
        file_name =
            "pixelgen_" + payload_leaf(options.payload) + "_" +
            options.transport + "_" + compact_timestamp() + extension;
    } else {
        const std::filesystem::path provided_name(file_name);
        if (provided_name.extension().empty()) {
            file_name += extension;
        }
    }

    const std::filesystem::path directory =
        options.output_dir.empty()
            ? std::filesystem::current_path()
            : std::filesystem::path(options.output_dir);
    return directory / file_name;
}

std::string build_simulation_image_code(const CliOptions& options) {
    std::ostringstream out;
    out << "tool=pixelgen;"
        << "mode=simulation_only;"
        << "created_at=" << now_string() << ";"
        << "payload=" << options.payload << ";"
        << "encoder=" << (options.encoder.empty() ? "none" : options.encoder) << ";"
        << "transport=" << options.transport << ";"
        << "transport_module=" << transport_module_id(options.transport) << ";"
        << "platform=" << options.platform << ";"
        << "iterations=" << options.iterations << ";"
        << "rhost=" << (options.rhost.empty() ? "none" : options.rhost) << ";"
        << "lhost=" << (options.lhost.empty() ? "none" : options.lhost) << ";"
        << "lport=" << options.lport;
    return out.str();
}

bool report_path_check(const pixelferrite::path_verifier::CheckResult& result) {
    if (result.ok) {
        return true;
    }

    std::cerr << pixelferrite::colors::wrap("Path verifier:", pixelferrite::colors::error())
              << " " << pixelferrite::colors::wrap(result.detail, pixelferrite::colors::warning()) << '\n';
    return false;
}

std::string resolve_modules_root(std::string modules_root) {
    const std::filesystem::path direct(modules_root);
    if (std::filesystem::exists(direct)) {
        return direct.generic_string();
    }

    if (const char* home = std::getenv("PF_HOME"); home != nullptr && *home != '\0') {
        const std::filesystem::path from_home = std::filesystem::path(home) / modules_root;
        if (std::filesystem::exists(from_home)) {
            return from_home.generic_string();
        }
    }

    const std::filesystem::path exe = executable_path();
    if (!exe.empty()) {
        const std::filesystem::path from_exe_parent = exe.parent_path().parent_path() / modules_root;
        if (std::filesystem::exists(from_exe_parent)) {
            return from_exe_parent.generic_string();
        }
        const std::filesystem::path from_exe_grandparent =
            exe.parent_path().parent_path().parent_path() / modules_root;
        if (std::filesystem::exists(from_exe_grandparent)) {
            return from_exe_grandparent.generic_string();
        }
    }

    return modules_root;
}

bool verify_startup_paths(
    const CliOptions& options,
    const std::string& modules_root,
    bool require_modules_root,
    bool require_output_path,
    const std::filesystem::path& image_output_path
) {
    bool ok = true;
    const std::filesystem::path data_root = runtime_data_root();
    const std::filesystem::path workspace_root = data_root / "workspace" / "default";

    ok = report_path_check(pixelferrite::path_verifier::ensure_directory(data_root, "pixelgen data root")) && ok;
    ok = report_path_check(pixelferrite::path_verifier::ensure_directory(workspace_root, "pixelgen workspace root")) && ok;
    ok = report_path_check(pixelferrite::path_verifier::ensure_directory(workspace_root / "tmp", "pixelgen workspace tmp")) && ok;
    if (require_modules_root) {
        ok = report_path_check(
            pixelferrite::path_verifier::require_existing_directory(modules_root, "modules root")
        ) && ok;
    }

    if (!options.input_image.empty()) {
        ok = report_path_check(
            pixelferrite::path_verifier::require_existing_file(options.input_image, "input image")
        ) && ok;
    }

    if (normalize_transport(options.transport).empty()) {
        std::cerr << pixelferrite::colors::wrap("Unsupported transport profile:", pixelferrite::colors::error())
                  << " " << pixelferrite::colors::wrap(options.transport, pixelferrite::colors::warning())
                  << " (use tcp|http|https|dns|quic|tls|file)\n";
        ok = false;
    }

    if (require_output_path) {
        const std::filesystem::path output_file =
            options.output.empty() ? default_artifact_path() : std::filesystem::path(options.output);
        ok = report_path_check(
            pixelferrite::path_verifier::require_writable_file_path(output_file, "output artifact")
        ) && ok;
    }

    if (!image_output_path.empty()) {
        ok = report_path_check(
            pixelferrite::path_verifier::require_writable_file_path(image_output_path, "embedded image output")
        ) && ok;
    }

    return ok;
}

bool matches_target(const pixelferrite::ModuleDescriptor& module, const std::string& target) {
    static const std::unordered_map<std::string, std::string> target_map = {
        {"payloads", "payload"},
        {"exploits", "exploit"},
        {"encoders", "encoder"},
        {"evasion", "evasion"},
        {"nops", "nop"},
        {"transports", "transport"},
        {"detection", "detection"},
        {"analysis", "analysis"},
        {"auxiliary", "auxiliary"},
        {"lab", "lab"}
    };

    const auto found = target_map.find(target);
    if (found == target_map.end()) {
        return false;
    }
    return module.category == found->second;
}

void list_modules(const std::vector<pixelferrite::ModuleDescriptor>& modules, const std::string& target) {
    if (target == "formats") {
        std::cout << "json\ntxt\nyaml\n";
        return;
    }

    if (target == "all") {
        for (const auto& module : modules) {
            std::cout << pixelferrite::colors::wrap(module.id, pixelferrite::colors::for_category(module.category))
                      << " [" << pixelferrite::colors::wrap(module.category, pixelferrite::colors::dim()) << "]\n";
        }
        std::cout << "formats: json, txt, yaml\n";
        return;
    }

    if (target != "payloads" && target != "exploits" && target != "encoders" &&
        target != "evasion" && target != "nops" && target != "transports" &&
        target != "detection" && target != "analysis" && target != "auxiliary" &&
        target != "lab") {
        std::cerr << pixelferrite::colors::wrap("Unsupported list target:", pixelferrite::colors::error())
                  << " " << pixelferrite::colors::wrap(target, pixelferrite::colors::warning()) << '\n';
        return;
    }

    for (const auto& module : modules) {
        if (!matches_target(module, target)) {
            continue;
        }
        std::cout << pixelferrite::colors::wrap(module.id, pixelferrite::colors::for_category(module.category));
        if (!module.name.empty()) {
            std::cout << " - " << pixelferrite::colors::wrap(module.name, pixelferrite::colors::bold());
        }
        std::cout << '\n';
    }
}

bool parse_args(int argc, char** argv, CliOptions& options) {
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];

        auto read_value = [&](const char* flag) -> std::string {
            if (i + 1 >= argc) {
                std::cerr << pixelferrite::colors::wrap("Missing value for", pixelferrite::colors::error())
                          << " " << pixelferrite::colors::wrap(flag, pixelferrite::colors::warning()) << '\n';
                return {};
            }
            return argv[++i];
        };

        if (arg == "-h" || arg == "--help") {
            options.help_requested = true;
            return true;
        } else if (arg == "-l" || arg == "--list") {
            options.list_target = read_value(arg.c_str());
            if (options.list_target.empty()) {
                return false;
            }
            std::transform(options.list_target.begin(), options.list_target.end(), options.list_target.begin(), [](unsigned char ch) {
                return static_cast<char>(std::tolower(ch));
            });
        } else if (arg == "-p" || arg == "--payload") {
            options.payload = read_value(arg.c_str());
            if (options.payload.empty()) {
                return false;
            }
        } else if (arg == "-e" || arg == "--encoder") {
            options.encoder = read_value(arg.c_str());
            if (options.encoder.empty()) {
                return false;
            }
        } else if (arg == "-t" || arg == "--transport") {
            options.transport = read_value(arg.c_str());
            if (options.transport.empty()) {
                return false;
            }
            options.transport = normalize_transport(options.transport);
            if (options.transport.empty()) {
                std::cerr << pixelferrite::colors::wrap("Unsupported transport profile.", pixelferrite::colors::error())
                          << " Use tcp|http|https|dns|quic|tls|file\n";
                return false;
            }
        } else if (arg == "-f" || arg == "--format") {
            options.format = read_value(arg.c_str());
            if (options.format.empty()) {
                return false;
            }
        } else if (arg == "-o" || arg == "--out") {
            options.output = read_value(arg.c_str());
            if (options.output.empty()) {
                return false;
            }
        } else if (arg == "--input") {
            options.input_image = read_value(arg.c_str());
            if (options.input_image.empty()) {
                return false;
            }
        } else if (arg == "--carrier") {
            options.input_image = read_value(arg.c_str());
            if (options.input_image.empty()) {
                return false;
            }
        } else if (arg == "--image-out") {
            options.image_output = read_value(arg.c_str());
            if (options.image_output.empty()) {
                return false;
            }
        } else if (arg == "--name") {
            options.output_name = read_value(arg.c_str());
            if (options.output_name.empty()) {
                return false;
            }
        } else if (arg == "--dir") {
            options.output_dir = read_value(arg.c_str());
            if (options.output_dir.empty()) {
                return false;
            }
        } else if (arg == "--lhost") {
            options.lhost = read_value(arg.c_str());
            if (options.lhost.empty()) {
                return false;
            }
        } else if (arg == "--rhost") {
            options.rhost = read_value(arg.c_str());
            if (options.rhost.empty()) {
                return false;
            }
        } else if (arg == "--lport") {
            const std::string raw = read_value(arg.c_str());
            if (raw.empty()) {
                return false;
            }
            try {
                options.lport = std::stoi(raw);
                if (options.lport < 0 || options.lport > 65535) {
                    throw std::out_of_range("invalid");
                }
            } catch (...) {
                std::cerr << pixelferrite::colors::wrap("Invalid lport value.", pixelferrite::colors::error()) << '\n';
                return false;
            }
        } else if (arg == "--platform") {
            options.platform = read_value(arg.c_str());
            if (options.platform.empty()) {
                return false;
            }
        } else if (arg == "--iterations") {
            const std::string raw = read_value(arg.c_str());
            if (raw.empty()) {
                return false;
            }
            try {
                options.iterations = std::max(1, std::stoi(raw));
            } catch (...) {
                std::cerr << pixelferrite::colors::wrap("Invalid iterations value.", pixelferrite::colors::error()) << '\n';
                return false;
            }
        } else {
            std::cerr << pixelferrite::colors::wrap("Unknown option:", pixelferrite::colors::error())
                      << " " << pixelferrite::colors::wrap(arg, pixelferrite::colors::warning()) << '\n';
            return false;
        }
    }

    return true;
}

bool write_artifact(const CliOptions& options) {
    const std::filesystem::path output_file =
        options.output.empty() ? default_artifact_path() : std::filesystem::path(options.output);
    const std::string out_path = output_file.generic_string();

    const auto writable =
        pixelferrite::path_verifier::require_writable_file_path(output_file, "output artifact");
    if (!writable.ok) {
        std::cerr << pixelferrite::colors::wrap("Unable to write output file:", pixelferrite::colors::error())
                  << " " << pixelferrite::colors::wrap(writable.detail, pixelferrite::colors::warning()) << '\n';
        return false;
    }

    std::ofstream out(output_file, std::ios::trunc);
    if (!out) {
        std::cerr << pixelferrite::colors::wrap("Unable to write output file:", pixelferrite::colors::error())
                  << " " << pixelferrite::colors::wrap(out_path, pixelferrite::colors::warning()) << '\n';
        return false;
    }

    if (options.format == "json") {
        out << "{\n"
            << "  \"tool\": \"pixelgen\",\n"
            << "  \"mode\": \"simulation_only\",\n"
            << "  \"created_at\": \"" << now_string() << "\",\n"
            << "  \"payload\": \"" << options.payload << "\",\n"
            << "  \"encoder\": \"" << (options.encoder.empty() ? "none" : options.encoder) << "\",\n"
            << "  \"transport\": \"" << options.transport << "\",\n"
            << "  \"transport_module\": \"" << transport_module_id(options.transport) << "\",\n"
            << "  \"platform\": \"" << options.platform << "\",\n"
            << "  \"iterations\": " << options.iterations << ",\n"
            << "  \"input_image\": \"" << (options.input_image.empty() ? "none" : options.input_image) << "\",\n"
            << "  \"lhost\": \"" << (options.lhost.empty() ? "none" : options.lhost) << "\",\n"
            << "  \"lport\": " << options.lport << ",\n"
            << "  \"rhost\": \"" << (options.rhost.empty() ? "none" : options.rhost) << "\"\n"
            << "}\n";
    } else if (options.format == "txt") {
        out << "pixelgen simulation artifact\n";
        out << "created_at=" << now_string() << '\n';
        out << "payload=" << options.payload << '\n';
        out << "encoder=" << (options.encoder.empty() ? "none" : options.encoder) << '\n';
        out << "transport=" << options.transport << '\n';
        out << "transport_module=" << transport_module_id(options.transport) << '\n';
        out << "platform=" << options.platform << '\n';
        out << "iterations=" << options.iterations << '\n';
        out << "input_image=" << (options.input_image.empty() ? "none" : options.input_image) << '\n';
        out << "lhost=" << (options.lhost.empty() ? "none" : options.lhost) << '\n';
        out << "lport=" << options.lport << '\n';
        out << "rhost=" << (options.rhost.empty() ? "none" : options.rhost) << '\n';
    } else if (options.format == "yaml") {
        out << "tool: pixelgen\n";
        out << "mode: simulation_only\n";
        out << "created_at: \"" << now_string() << "\"\n";
        out << "payload: \"" << options.payload << "\"\n";
        out << "encoder: \"" << (options.encoder.empty() ? "none" : options.encoder) << "\"\n";
        out << "transport: \"" << options.transport << "\"\n";
        out << "transport_module: \"" << transport_module_id(options.transport) << "\"\n";
        out << "platform: \"" << options.platform << "\"\n";
        out << "iterations: " << options.iterations << '\n';
        out << "input_image: \"" << (options.input_image.empty() ? "none" : options.input_image) << "\"\n";
        out << "lhost: \"" << (options.lhost.empty() ? "none" : options.lhost) << "\"\n";
        out << "lport: " << options.lport << '\n';
        out << "rhost: \"" << (options.rhost.empty() ? "none" : options.rhost) << "\"\n";
    } else {
        std::cerr << pixelferrite::colors::wrap("Unsupported format:", pixelferrite::colors::error())
                  << " " << pixelferrite::colors::wrap(options.format, pixelferrite::colors::warning()) << '\n';
        return false;
    }

    std::cout << pixelferrite::colors::wrap("[+] Simulation artifact written to", pixelferrite::colors::success())
              << " " << pixelferrite::colors::wrap(out_path, pixelferrite::colors::info()) << '\n';
    return true;
}

bool write_simulation_image(const CliOptions& options, const std::filesystem::path& output_image) {
    if (options.input_image.empty()) {
        return true;
    }

    const std::filesystem::path input_image(options.input_image);
    if (input_image.lexically_normal() == output_image.lexically_normal()) {
        std::cerr << pixelferrite::colors::wrap("Input and output image path cannot be the same.", pixelferrite::colors::error())
                  << '\n';
        return false;
    }

    pixelferrite::ImageEngine image_engine;
    if (!image_engine.supports_format(input_image)) {
        std::cerr << pixelferrite::colors::wrap("Unsupported carrier format.", pixelferrite::colors::error())
                  << " " << pixelferrite::colors::wrap(image_engine.supported_extensions(), pixelferrite::colors::warning())
                  << '\n';
        return false;
    }

    const auto writable =
        pixelferrite::path_verifier::require_writable_file_path(output_image, "embedded image output");
    if (!writable.ok) {
        std::cerr << pixelferrite::colors::wrap("Unable to write embedded image:", pixelferrite::colors::error())
                  << " " << pixelferrite::colors::wrap(writable.detail, pixelferrite::colors::warning()) << '\n';
        return false;
    }

    const std::string simulation_code = build_simulation_image_code(options);
    const auto embed_report = image_engine.embed_simulation_code(input_image, output_image, simulation_code);
    if (!embed_report.success) {
        std::cerr << pixelferrite::colors::wrap("Image embed failed:", pixelferrite::colors::error())
                  << " " << pixelferrite::colors::wrap(embed_report.message, pixelferrite::colors::warning()) << '\n';
        return false;
    }

    std::cout << pixelferrite::colors::wrap("[+] Simulation image written to", pixelferrite::colors::success())
              << " " << pixelferrite::colors::wrap(output_image.generic_string(), pixelferrite::colors::info()) << '\n';
    std::cout << pixelferrite::colors::wrap("[*] format", pixelferrite::colors::accent()) << "=" << embed_report.format
              << " " << pixelferrite::colors::wrap("integrity", pixelferrite::colors::accent()) << "="
              << (embed_report.integrity_ok ? "ok" : "failed")
              << " size_in=" << embed_report.input_size
              << " size_out=" << embed_report.output_size << '\n';
    return true;
}

}  // namespace

int main(int argc, char** argv) {
    pixelferrite::colors::initialize();
    CliOptions options;
    if (argc <= 1) {
        print_help(argv[0]);
        return 1;
    }

    if (!parse_args(argc, argv, options)) {
        print_help(argv[0]);
        return 1;
    }

    if (options.help_requested) {
        print_help(argv[0]);
        return 0;
    }

    const std::filesystem::path image_output_path = resolve_image_output_path(options);
    const std::string modules_root = resolve_modules_root("modules");
    const bool requires_modules = options.list_target.empty() || options.list_target != "formats";
    const bool requires_output_path = options.list_target.empty();
    if (!verify_startup_paths(options, modules_root, requires_modules, requires_output_path, image_output_path)) {
        return 1;
    }

    pixelferrite::ModuleLoader loader;
    const auto modules = loader.discover(modules_root);

    if (!options.list_target.empty()) {
        list_modules(modules, options.list_target);
        return 0;
    }

    if (options.payload.empty()) {
        print_help(argv[0]);
        return 1;
    }

    const bool image_ok = write_simulation_image(options, image_output_path);
    const bool artifact_ok = write_artifact(options);
    return (image_ok && artifact_ok) ? 0 : 1;
}


