#include <pixelferrite/console.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
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

#include <pixelferrite/banner.hpp>
#include <pixelferrite/colors.hpp>
#include <pixelferrite/logger.hpp>

namespace pixelferrite {

namespace {

constexpr std::string_view kVersion = "0.1.0";

std::string hdr(std::string_view text) {
    return colors::wrap(text, colors::accent());
}

std::string info_msg(std::string_view text) {
    return colors::wrap(text, colors::info());
}

std::string ok_msg(std::string_view text) {
    return colors::wrap(text, colors::success());
}

std::string warn_msg(std::string_view text) {
    return colors::wrap(text, colors::warning());
}

std::string err_msg(std::string_view text) {
    return colors::wrap(text, colors::error());
}

std::string to_lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

std::string to_upper(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::toupper(ch));
    });
    return value;
}

std::string now_string(const char* format) {
    const auto now = std::chrono::system_clock::now();
    const std::time_t time = std::chrono::system_clock::to_time_t(now);

    std::tm local_time{};
#if defined(_WIN32)
    localtime_s(&local_time, &time);
#else
    localtime_r(&time, &local_time);
#endif

    std::ostringstream stream;
    stream << std::put_time(&local_time, format);
    return stream.str();
}

void append_line(const std::filesystem::path& path, const std::string& line) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::app);
    if (output) {
        output << line << '\n';
    }
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

std::filesystem::path executable_dir() {
    const auto exe = executable_path();
    if (exe.empty()) {
        return std::filesystem::current_path();
    }
    return exe.parent_path();
}

std::filesystem::path home_dir() {
#if defined(_WIN32)
    if (const char* profile = std::getenv("USERPROFILE"); profile != nullptr && *profile != '\0') {
        return std::filesystem::path(profile);
    }
    const char* home_drive = std::getenv("HOMEDRIVE");
    const char* home_path = std::getenv("HOMEPATH");
    if (home_drive != nullptr && home_path != nullptr) {
        return std::filesystem::path(std::string(home_drive) + std::string(home_path));
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

std::filesystem::path runtime_logs_dir() {
    return runtime_data_root() / "logs";
}

std::filesystem::path runtime_config_dir() {
    return runtime_data_root() / "config";
}

std::filesystem::path runtime_workspace_root_dir() {
    return runtime_data_root() / "workspace";
}

std::filesystem::path runtime_workspace_dir(std::string_view workspace) {
    return runtime_workspace_root_dir() / std::string(workspace);
}

std::filesystem::path runtime_workspace_logs_dir(std::string_view workspace) {
    return runtime_workspace_dir(workspace) / "logs";
}

std::filesystem::path runtime_workspace_reports_dir(std::string_view workspace) {
    return runtime_workspace_dir(workspace) / "reports";
}

std::filesystem::path runtime_workspace_sessions_dir(std::string_view workspace) {
    return runtime_workspace_dir(workspace) / "sessions";
}

std::filesystem::path runtime_workspace_scripts_dir(std::string_view workspace) {
    return runtime_workspace_dir(workspace) / "scripts";
}

std::filesystem::path runtime_workspace_state_dir(std::string_view workspace) {
    return runtime_workspace_dir(workspace) / "state";
}

std::filesystem::path runtime_workspace_tmp_dir(std::string_view workspace) {
    return runtime_workspace_dir(workspace) / "tmp";
}

std::filesystem::path runtime_workspace_dataset_dir(std::string_view workspace) {
    return runtime_workspace_dir(workspace) / "datasets";
}

void ensure_workspace_layout(std::string_view workspace) {
    std::filesystem::create_directories(runtime_logs_dir());
    std::filesystem::create_directories(runtime_config_dir());
    std::filesystem::create_directories(runtime_workspace_logs_dir(workspace));
    std::filesystem::create_directories(runtime_workspace_reports_dir(workspace));
    std::filesystem::create_directories(runtime_workspace_sessions_dir(workspace));
    std::filesystem::create_directories(runtime_workspace_scripts_dir(workspace));
    std::filesystem::create_directories(runtime_workspace_state_dir(workspace));
    std::filesystem::create_directories(runtime_workspace_tmp_dir(workspace));
    std::filesystem::create_directories(runtime_workspace_dataset_dir(workspace));
}

std::string resolve_modules_root(std::string modules_root) {
    const std::filesystem::path root_candidate(modules_root);
    if (root_candidate.is_absolute() && std::filesystem::exists(root_candidate)) {
        return root_candidate.generic_string();
    }
    if (std::filesystem::exists(root_candidate)) {
        return root_candidate.generic_string();
    }

    if (const char* home = std::getenv("PF_HOME"); home != nullptr && *home != '\0') {
        const std::filesystem::path from_home = std::filesystem::path(home) / modules_root;
        if (std::filesystem::exists(from_home)) {
            return from_home.generic_string();
        }
    }

    const std::filesystem::path exe_dir = executable_dir();
    const std::filesystem::path from_exe_parent = exe_dir.parent_path() / modules_root;
    if (std::filesystem::exists(from_exe_parent)) {
        return from_exe_parent.generic_string();
    }

    const std::filesystem::path from_exe_grandparent = exe_dir.parent_path().parent_path() / modules_root;
    if (std::filesystem::exists(from_exe_grandparent)) {
        return from_exe_grandparent.generic_string();
    }

    return modules_root;
}

void ensure_runtime_layout() {
    ensure_workspace_layout("default");
}

void log_framework_line(const std::string& message) {
    append_line(runtime_logs_dir() / "framework.log", now_string("%Y-%m-%d %H:%M:%S") + " " + message);
}

void log_session_line(const std::string& message) {
    append_line(runtime_logs_dir() / "sessions.log", now_string("%Y-%m-%d %H:%M:%S") + " " + message);
}

void print_module_table(const std::vector<ModuleDescriptor>& modules) {
    if (modules.empty()) {
        std::cout << warn_msg("No modules found.") << '\n';
        return;
    }

    for (const ModuleDescriptor& module : modules) {
        std::cout << "  "
                  << colors::wrap(module.id, colors::for_category(module.category))
                  << " ["
                  << colors::wrap(module.category, colors::dim())
                  << "]";
        if (!module.name.empty()) {
            std::cout << " - " << colors::wrap(module.name, colors::bold());
        }
        if (!module.supported_platforms.empty()) {
            std::cout << " (" << colors::wrap("platforms:", colors::dim()) << " ";
            for (std::size_t i = 0; i < module.supported_platforms.size(); ++i) {
                if (i > 0) {
                    std::cout << ",";
                }
                std::cout << colors::wrap(module.supported_platforms[i], colors::info());
            }
            std::cout << ")";
        }
        std::cout << '\n';
    }
}

std::string short_module_name(const std::string& module_id) {
    const std::size_t pos = module_id.find_last_of('/');
    return (pos == std::string::npos) ? module_id : module_id.substr(pos + 1);
}

std::string normalize_module_id(std::string module_id) {
    module_id = to_lower(std::move(module_id));
    const std::string prefix = "modules/";
    if (module_id.rfind(prefix, 0) == 0) {
        module_id = module_id.substr(prefix.size());
    }
    return module_id;
}

std::string normalize_tool_name(std::string tool) {
    tool = to_lower(std::move(tool));
    if (tool == "nmap.exe") {
        return "nmap";
    }
    if (tool == "netprobe-rs.exe") {
        return "netprobe-rs";
    }
    return tool;
}

bool is_supported_external_tool(const std::string& tool) {
    const std::string normalized = normalize_tool_name(tool);
    return normalized == "nmap" || normalized == "netprobe-rs";
}

std::string read_text_file(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::in | std::ios::binary);
    if (!input) {
        return {};
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

std::vector<std::string> split_targets(const std::string& raw) {
    std::vector<std::string> targets;
    std::string current;

    for (char ch : raw) {
        if (ch == ',' || std::isspace(static_cast<unsigned char>(ch))) {
            if (!current.empty()) {
                targets.push_back(current);
                current.clear();
            }
            continue;
        }
        current.push_back(ch);
    }

    if (!current.empty()) {
        targets.push_back(current);
    }

    return targets;
}

std::string detect_ip_version(const std::string& endpoint) {
    if (endpoint.find(':') != std::string::npos) {
        return "ipv6";
    }
    if (endpoint.find('.') != std::string::npos) {
        return "ipv4";
    }
    return "label";
}

std::string get_option_value(
    const std::unordered_map<std::string, std::string>& local_options,
    const std::unordered_map<std::string, std::string>& global_options,
    const std::string& key
) {
    const auto local_it = local_options.find(key);
    if (local_it != local_options.end()) {
        return local_it->second;
    }

    const auto global_it = global_options.find(key);
    if (global_it != global_options.end()) {
        return global_it->second;
    }

    return {};
}

std::vector<std::string> find_missing_required_options(
    const ModuleDescriptor& descriptor,
    const std::unordered_map<std::string, std::string>& local_options,
    const std::unordered_map<std::string, std::string>& global_options
) {
    std::vector<std::string> missing;
    for (const ModuleOption& option : descriptor.options) {
        if (!option.required) {
            continue;
        }

        const std::string key = to_upper(option.name);
        std::string value = get_option_value(local_options, global_options, key);
        if (value.empty()) {
            value = option.default_value;
        }

        if (value.empty()) {
            missing.push_back(key);
        }
    }
    return missing;
}

std::string compact_timestamp() {
    return now_string("%Y%m%d_%H%M%S");
}

std::string capture_command_output(const std::string& command) {
    std::string output;
#if defined(_WIN32)
    FILE* pipe = _popen(command.c_str(), "r");
#else
    FILE* pipe = popen(command.c_str(), "r");
#endif
    if (pipe == nullptr) {
        return {};
    }

    char buffer[512];
    while (std::fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output += buffer;
    }

#if defined(_WIN32)
    _pclose(pipe);
#else
    pclose(pipe);
#endif
    return output;
}

int run_external_command_stream(const std::string& command) {
#if defined(_WIN32)
    const std::string wrapped = command + " 2>&1";
    FILE* pipe = _popen(wrapped.c_str(), "r");
#else
    const std::string wrapped = command + " 2>&1";
    FILE* pipe = popen(wrapped.c_str(), "r");
#endif
    if (pipe == nullptr) {
        std::cout << "[-] unable to start command: " << command << '\n';
        return -1;
    }

    char buffer[1024];
    while (std::fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        std::cout << buffer;
    }

#if defined(_WIN32)
    return _pclose(pipe);
#else
    return pclose(pipe);
#endif
}

bool is_target_in_scope(const std::vector<std::string>& allowed_targets, const std::string& target) {
    if (allowed_targets.empty()) {
        return true;
    }

    for (const std::string& allowed : allowed_targets) {
        if (allowed == target || allowed == "*") {
            return true;
        }
        if (!allowed.empty() && allowed.back() == '*') {
            const std::string prefix = allowed.substr(0, allowed.size() - 1);
            if (target.rfind(prefix, 0) == 0) {
                return true;
            }
        }
    }

    return false;
}

std::string join_lines(const std::vector<std::string>& lines) {
    std::ostringstream stream;
    for (const std::string& line : lines) {
        stream << line << '\n';
    }
    return stream.str();
}

std::string run_local_device_inventory() {
    std::vector<std::string> lines;
    lines.push_back("collector=local_system_inventory");
    lines.push_back("timestamp=" + now_string("%Y-%m-%d %H:%M:%S"));
    lines.push_back("hostname=" + capture_command_output("hostname"));
#if defined(_WIN32)
    lines.push_back("os=" + capture_command_output("ver"));
    lines.push_back("cpu=" + capture_command_output("powershell -NoProfile -Command \"(Get-CimInstance Win32_Processor | Select-Object -First 1 Name).Name\""));
    lines.push_back("memory_mb=" + capture_command_output("powershell -NoProfile -Command \"[math]::Round((Get-CimInstance Win32_OperatingSystem).TotalVisibleMemorySize/1024,2)\""));
#else
    lines.push_back("os=" + capture_command_output("uname -a"));
    lines.push_back("cpu=" + capture_command_output("uname -p"));
    lines.push_back("memory=" + capture_command_output("free -m 2>/dev/null"));
#endif
    return join_lines(lines);
}

std::string run_local_cpu_memory_profile() {
    std::vector<std::string> lines;
    lines.push_back("collector=cpu_memory_profile");
    lines.push_back("timestamp=" + now_string("%Y-%m-%d %H:%M:%S"));
#if defined(_WIN32)
    lines.push_back("processor=" + capture_command_output("powershell -NoProfile -Command \"Get-CimInstance Win32_Processor | Select-Object Name,NumberOfCores,NumberOfLogicalProcessors | ConvertTo-Json -Compress\""));
    lines.push_back("memory=" + capture_command_output("powershell -NoProfile -Command \"Get-CimInstance Win32_OperatingSystem | Select-Object TotalVisibleMemorySize,FreePhysicalMemory | ConvertTo-Json -Compress\""));
#else
    lines.push_back("cpu=" + capture_command_output("lscpu 2>/dev/null"));
    lines.push_back("memory=" + capture_command_output("free -h 2>/dev/null"));
#endif
    return join_lines(lines);
}

std::string run_local_log_snapshot() {
    std::vector<std::string> lines;
    lines.push_back("collector=local_log_snapshot");
    lines.push_back("timestamp=" + now_string("%Y-%m-%d %H:%M:%S"));
#if defined(_WIN32)
    lines.push_back(capture_command_output("powershell -NoProfile -Command \"Get-WinEvent -LogName System -MaxEvents 15 | Select-Object TimeCreated,Id,LevelDisplayName,ProviderName,Message | Format-List\""));
#else
    lines.push_back(capture_command_output("journalctl -n 15 --no-pager 2>/dev/null"));
#endif
    return join_lines(lines);
}

std::string run_hypervisor_inventory() {
    std::vector<std::string> lines;
    lines.push_back("collector=hypervisor_inventory");
    lines.push_back("timestamp=" + now_string("%Y-%m-%d %H:%M:%S"));
#if defined(_WIN32)
    lines.push_back(capture_command_output("powershell -NoProfile -Command \"Get-VM | Select-Object Name,State,CPUUsage,MemoryAssigned,Uptime | ConvertTo-Json -Depth 4\""));
#else
    lines.push_back("Hypervisor inventory is only implemented for Windows Hyper-V in this build.");
#endif
    return join_lines(lines);
}

std::string run_ping_probe(const std::vector<std::string>& targets) {
    std::vector<std::string> lines;
    lines.push_back("collector=reachability_probe");
    lines.push_back("timestamp=" + now_string("%Y-%m-%d %H:%M:%S"));

    for (const std::string& target : targets) {
#if defined(_WIN32)
        const std::string command = "ping -n 1 " + target;
#else
        const std::string command = "ping -c 1 " + target;
#endif
        const std::string output = capture_command_output(command);
        const bool reachable = output.find("TTL=") != std::string::npos || output.find("ttl=") != std::string::npos ||
                               output.find("1 received") != std::string::npos;
        lines.push_back(target + "=" + (reachable ? "reachable" : "unreachable"));
    }

    return join_lines(lines);
}

}  // namespace

ConsoleEngine::ConsoleEngine(std::string modules_root)
    : modules_root_(resolve_modules_root(std::move(modules_root))),
      modules_(module_loader_.discover(modules_root_)) {
    colors::initialize();
    ensure_runtime_layout();
    ensure_workspace_layout(active_workspace_);
    allowed_tools_by_workspace_[active_workspace_] = {"nmap", "netprobe-rs"};
    denied_tools_by_workspace_[active_workspace_] = {};
}

void ConsoleEngine::show_banner() const {
    std::cout << colors::wrap(banner_text(), colors::brand()) << '\n';
    std::cout << colors::wrap("author", colors::brand())
              << " = " << colors::wrap("voltsparx", colors::orange()) << '\n';
    std::cout << colors::wrap("contact", colors::brand())
              << " = " << colors::wrap("voltsparx@gmail.com", colors::gray()) << '\n';
}

void ConsoleEngine::run_repl() {
    constexpr auto kMinStartupDelay = std::chrono::milliseconds(900);
    constexpr auto kSpinnerTick = std::chrono::milliseconds(75);
    constexpr std::array<char, 4> kSpinner = {'/', '-', '\\', '|'};

    const auto load_start = std::chrono::steady_clock::now();
    modules_ = module_loader_.discover(modules_root_);
    const auto load_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - load_start
    );

    if (load_elapsed < kMinStartupDelay) {
        auto remaining = kMinStartupDelay - load_elapsed;
        std::size_t frame = 0;

        while (remaining.count() > 0) {
            std::cout << "\r" << info_msg("[*] Loading PixelFerrite modules")
                      << " " << colors::wrap(std::string(1, kSpinner[frame % kSpinner.size()]), colors::accent())
                      << std::flush;
            const auto sleep_slice = (remaining < kSpinnerTick) ? remaining : kSpinnerTick;
            std::this_thread::sleep_for(sleep_slice);
            remaining -= sleep_slice;
            ++frame;
        }
    }

    std::cout << "\r" << ok_msg("[*] Loading PixelFerrite modules done.") << "               \n";
    std::cout << info_msg("[*] Module count:")
              << " " << colors::wrap(std::to_string(modules_.size()), colors::bold())
              << " | " << colors::wrap("init:", colors::dim())
              << " " << colors::wrap(std::to_string(load_elapsed.count()) + "ms", colors::accent())
              << '\n';

    Logger::info("pffconsole initialized");
    std::cout << info_msg("pff> type 'help' for commands, 'exit' to quit") << '\n';

    std::string line;
    while (running_ && std::cout << prompt() && std::getline(std::cin, line)) {
        if (line.empty()) {
            continue;
        }

        history_.push_back(line);
        log_framework_line("command=" + line);
        handle_command(line);
    }
}

bool ConsoleEngine::handle_command(const std::string& line) {
    const std::vector<std::string> tokens = tokenize(line);
    if (tokens.empty()) {
        return true;
    }

    if (spool_enabled_) {
        append_spool_line("pff> " + line);
    }

    const std::string command = to_lower(tokens[0]);

    if (command == "help" || command == "?") {
        print_help();
        return true;
    }
    if (command == "banner") {
        show_banner();
        return true;
    }
    if (command == "version") {
        std::cout << colors::wrap("PixelFerrite Framework v", colors::brand())
                  << colors::wrap(std::string(kVersion), colors::accent()) << '\n';
        return true;
    }
    if (command == "clear") {
#if defined(_WIN32)
        (void)std::system("cls");
#else
        (void)std::system("clear");
#endif
        return true;
    }
    if (command == "history") {
        for (std::size_t i = 0; i < history_.size(); ++i) {
            std::cout << std::setw(4) << (i + 1) << "  " << history_[i] << '\n';
        }
        return true;
    }
    if (command == "exit" || command == "quit") {
        running_ = false;
        return true;
    }
    if (command == "workspace") {
        return handle_workspace(tokens);
    }
    if (command == "tools") {
        return handle_tools(tokens);
    }
    if (command == "scope") {
        return handle_scope(tokens);
    }
    if (command == "show") {
        return handle_show(tokens);
    }
    if (command == "search") {
        return handle_search(tokens);
    }
    if (command == "use") {
        return handle_use(tokens);
    }
    if (command == "back") {
        active_module_.clear();
        active_options_.clear();
        std::cout << "[*] Module context cleared.\n";
        return true;
    }
    if (command == "info") {
        return handle_info();
    }
    if (command == "set" || command == "setg") {
        const bool is_global = (command == "setg");
        if (tokens.size() < 3) {
            std::cout << "Usage: " << command << " <key> <value>\n";
            return true;
        }

        const std::string key = to_upper(tokens[1]);
        const std::string value = join(tokens, 2);
        if (is_global) {
            global_options_[key] = value;
            std::cout << key << " => " << value << " (global)\n";
            return true;
        }
        return handle_set(tokens);
    }
    if (command == "unset" || command == "unsetg") {
        const bool is_global = (command == "unsetg");
        if (tokens.size() < 2) {
            std::cout << "Usage: " << command << " <key|all>\n";
            return true;
        }

        if (is_global) {
            if (to_lower(tokens[1]) == "all") {
                global_options_.clear();
                std::cout << "[*] Cleared global options.\n";
                return true;
            }

            const std::size_t erased = global_options_.erase(to_upper(tokens[1]));
            std::cout << (erased ? "[*] Global option removed.\n" : "[-] Global option not set.\n");
            return true;
        }
        return handle_unset(tokens);
    }
    if (
        command == "run" || command == "exploit" || command == "check" ||
        command == "embed" || command == "extract" || command == "simulate" ||
        command == "analyze" || command == "mutate"
    ) {
        const std::string job_detail = command + " " + (active_module_.empty() ? "<no_module>" : active_module_);
        const std::string job_id = start_job("module", job_detail);
        const bool result = handle_run_like(command);
        finish_job(job_id, result ? "completed" : "failed");
        return result;
    }
    if (command == "sessions") {
        return handle_sessions(tokens);
    }
    if (command == "log") {
        return handle_log(tokens);
    }
    if (command == "report") {
        return handle_report(tokens);
    }
    if (command == "dataset") {
        return handle_dataset(tokens);
    }
    if (is_supported_external_tool(command)) {
        const std::string normalized_tool = normalize_tool_name(command);
        if (allowed_tools_by_workspace_.find(active_workspace_) == allowed_tools_by_workspace_.end()) {
            allowed_tools_by_workspace_[active_workspace_] = {"nmap", "netprobe-rs"};
        }
        if (denied_tools_by_workspace_.find(active_workspace_) == denied_tools_by_workspace_.end()) {
            denied_tools_by_workspace_[active_workspace_] = {};
        }

        const auto& denied = denied_tools_by_workspace_[active_workspace_];
        if (denied.find(normalized_tool) != denied.end()) {
            std::cout << "[-] tool blocked by workspace policy: " << normalized_tool << '\n';
            return true;
        }

        const auto& allowed = allowed_tools_by_workspace_[active_workspace_];
        if (allowed.find(normalized_tool) == allowed.end()) {
            std::cout << "[-] tool is not in allow list for workspace '" << active_workspace_ << "': " << normalized_tool << '\n';
            std::cout << "Use: tools allow " << normalized_tool << '\n';
            return true;
        }

        const std::string job_id = start_job("external_tool", line);
        log_framework_line("external_tool=" + line);
        const int exit_code = run_external_command_stream(line);
        finish_job(job_id, exit_code == 0 ? "completed" : "failed");
        std::cout << "\n[*] command exit code: " << exit_code << '\n';
        return true;
    }
    if (command == "reload_all") {
        const std::string job_id = start_job("framework", "reload_all");
        modules_ = module_loader_.discover(modules_root_);
        finish_job(job_id, "completed");
        std::cout << ok_msg("[*] Reloaded")
                  << " " << colors::wrap(std::to_string(modules_.size()), colors::bold())
                  << " " << colors::wrap("modules.", colors::info()) << '\n';
        return true;
    }
    if (command == "jobs") {
        return handle_jobs(tokens);
    }
    if (command == "resource") {
        return handle_resource(tokens);
    }
    if (command == "makerc") {
        return handle_makerc(tokens);
    }
    if (command == "save") {
        return handle_save(tokens);
    }
    if (command == "spool") {
        return handle_spool(tokens);
    }
    if (command == "threads") {
        return handle_threads(tokens);
    }
    if (command == "route") {
        return handle_route(tokens);
    }
    if (command == "connect") {
        return handle_connect(tokens);
    }
    if (command == "db_connect") {
        return handle_db_connect(tokens);
    }
    if (command == "db_status") {
        return handle_db_status();
    }
    if (
        command == "load" || command == "unload" || command == "irb" || command == "pry" ||
        command == "cd" || command == "color" || command == "debug" || command == "edit" ||
        command == "hosts" || command == "services" || command == "creds" || command == "loot" ||
        command == "notes" || command == "vulns"
    ) {
        std::cout << "[*] '" << command << "' is available as a simulation stub in this build.\n";
        return true;
    }

    std::cout << err_msg("[-] Unknown command:") << " " << colors::wrap(command, colors::warning()) << '\n';
    std::cout << warn_msg("Type 'help' for available commands.") << '\n';
    return false;
}

bool ConsoleEngine::handle_workspace(const std::vector<std::string>& tokens) {
    if (tokens.size() < 2) {
        std::cout << "Usage: workspace <list|add|select|delete> [name]\n";
        return true;
    }

    const std::string action = to_lower(tokens[1]);
    if (action == "list") {
        for (const std::string& workspace : workspaces_) {
            const bool is_active = (workspace == active_workspace_);
            std::cout << (is_active ? "* " : "  ") << workspace << '\n';
        }
        return true;
    }

    if (tokens.size() < 3) {
        std::cout << "Usage: workspace " << action << " <name>\n";
        return true;
    }

    const std::string name = tokens[2];
    if (action == "add") {
        if (std::find(workspaces_.begin(), workspaces_.end(), name) != workspaces_.end()) {
            std::cout << "[-] Workspace already exists.\n";
            return true;
        }
        workspaces_.push_back(name);
        ensure_workspace_layout(name);
        allowed_tools_by_workspace_[name] = {"nmap", "netprobe-rs"};
        denied_tools_by_workspace_[name] = {};
        std::cout << "[+] Workspace added: " << name << '\n';
        return true;
    }

    if (action == "select") {
        if (std::find(workspaces_.begin(), workspaces_.end(), name) == workspaces_.end()) {
            std::cout << "[-] Workspace not found: " << name << '\n';
            return true;
        }
        active_workspace_ = name;
        if (allowed_tools_by_workspace_.find(name) == allowed_tools_by_workspace_.end()) {
            allowed_tools_by_workspace_[name] = {"nmap", "netprobe-rs"};
        }
        if (denied_tools_by_workspace_.find(name) == denied_tools_by_workspace_.end()) {
            denied_tools_by_workspace_[name] = {};
        }
        ensure_workspace_layout(name);
        std::cout << "[*] Active workspace: " << name << '\n';
        return true;
    }

    if (action == "delete") {
        if (name == "default") {
            std::cout << "[-] Cannot delete default workspace.\n";
            return true;
        }
        if (name == active_workspace_) {
            std::cout << "[-] Cannot delete active workspace.\n";
            return true;
        }

        const auto old_size = workspaces_.size();
        workspaces_.erase(
            std::remove(workspaces_.begin(), workspaces_.end(), name),
            workspaces_.end()
        );
        allowed_tools_by_workspace_.erase(name);
        denied_tools_by_workspace_.erase(name);
        std::filesystem::remove_all(runtime_workspace_dir(name));

        std::cout << (workspaces_.size() != old_size ? "[*] Workspace deleted.\n" : "[-] Workspace not found.\n");
        return true;
    }

    std::cout << "[-] Unsupported workspace action: " << action << '\n';
    return false;
}

bool ConsoleEngine::handle_tools(const std::vector<std::string>& tokens) {
    if (allowed_tools_by_workspace_.find(active_workspace_) == allowed_tools_by_workspace_.end()) {
        allowed_tools_by_workspace_[active_workspace_] = {"nmap", "netprobe-rs"};
    }
    if (denied_tools_by_workspace_.find(active_workspace_) == denied_tools_by_workspace_.end()) {
        denied_tools_by_workspace_[active_workspace_] = {};
    }

    auto& allowed = allowed_tools_by_workspace_[active_workspace_];
    auto& denied = denied_tools_by_workspace_[active_workspace_];

    if (tokens.size() < 2) {
        std::cout << "Usage: tools <list|allow <tool>|deny <tool>|reset>\n";
        return true;
    }

    const std::string action = to_lower(tokens[1]);
    if (action == "list") {
        std::cout << "Workspace: " << active_workspace_ << '\n';
        std::cout << "Allowed tools:\n";
        if (allowed.empty()) {
            std::cout << "  <none>\n";
        } else {
            for (const std::string& item : allowed) {
                std::cout << "  " << item << '\n';
            }
        }

        std::cout << "Denied tools:\n";
        if (denied.empty()) {
            std::cout << "  <none>\n";
        } else {
            for (const std::string& item : denied) {
                std::cout << "  " << item << '\n';
            }
        }
        return true;
    }

    if (action == "reset") {
        allowed = {"nmap", "netprobe-rs"};
        denied.clear();
        std::cout << "[*] tool policy reset to defaults.\n";
        return true;
    }

    if ((action == "allow" || action == "deny") && tokens.size() < 3) {
        std::cout << "Usage: tools " << action << " <nmap|netprobe-rs>\n";
        return true;
    }

    if (action == "allow") {
        const std::string tool = normalize_tool_name(tokens[2]);
        if (!is_supported_external_tool(tool)) {
            std::cout << "[-] unsupported tool for passthrough policy: " << tokens[2] << '\n';
            return true;
        }
        denied.erase(tool);
        allowed.insert(tool);
        std::cout << "[+] allowed tool: " << tool << '\n';
        return true;
    }

    if (action == "deny") {
        const std::string tool = normalize_tool_name(tokens[2]);
        if (!is_supported_external_tool(tool)) {
            std::cout << "[-] unsupported tool for passthrough policy: " << tokens[2] << '\n';
            return true;
        }
        allowed.erase(tool);
        denied.insert(tool);
        std::cout << "[*] denied tool: " << tool << '\n';
        return true;
    }

    std::cout << "[-] unsupported tools action: " << action << '\n';
    return false;
}

bool ConsoleEngine::handle_scope(const std::vector<std::string>& tokens) {
    if (tokens.size() < 2) {
        std::cout << "Usage: scope <list|enforce on|enforce off|add <target>|remove <target>|clear>\n";
        return true;
    }

    const std::string action = to_lower(tokens[1]);
    if (action == "list") {
        std::cout << "enforce_scope=" << (enforce_scope_ ? "on" : "off") << '\n';
        if (allowed_targets_.empty()) {
            std::cout << "  <no explicit scope, all targets allowed>\n";
            return true;
        }
        for (const std::string& item : allowed_targets_) {
            std::cout << "  " << item << '\n';
        }
        return true;
    }

    if (action == "enforce") {
        if (tokens.size() < 3) {
            std::cout << "Usage: scope enforce <on|off>\n";
            return true;
        }
        const std::string mode = to_lower(tokens[2]);
        enforce_scope_ = (mode == "on");
        std::cout << "[*] scope enforcement " << (enforce_scope_ ? "enabled" : "disabled") << '\n';
        return true;
    }

    if (action == "add") {
        if (tokens.size() < 3) {
            std::cout << "Usage: scope add <target|prefix*>\n";
            return true;
        }
        const std::string target = tokens[2];
        if (std::find(allowed_targets_.begin(), allowed_targets_.end(), target) == allowed_targets_.end()) {
            allowed_targets_.push_back(target);
        }
        std::cout << "[+] scope target added: " << target << '\n';
        return true;
    }

    if (action == "remove") {
        if (tokens.size() < 3) {
            std::cout << "Usage: scope remove <target|prefix*>\n";
            return true;
        }
        const std::string target = tokens[2];
        const auto old_size = allowed_targets_.size();
        allowed_targets_.erase(std::remove(allowed_targets_.begin(), allowed_targets_.end(), target), allowed_targets_.end());
        std::cout << (allowed_targets_.size() != old_size ? "[*] scope target removed.\n" : "[-] scope target not found.\n");
        return true;
    }

    if (action == "clear") {
        allowed_targets_.clear();
        std::cout << "[*] scope cleared.\n";
        return true;
    }

    std::cout << "[-] Unsupported scope action: " << action << '\n';
    return false;
}

bool ConsoleEngine::handle_show(const std::vector<std::string>& tokens) const {
    if (tokens.size() < 2) {
        std::cout << "Usage: show <modules|payloads|exploits|auxiliary|encoders|evasion|nops|transports|detection|analysis|lab|options|platforms|sessions>\n";
        return true;
    }

    const std::string target = to_lower(tokens[1]);
    if (target == "modules") {
        print_module_table(modules_);
        return true;
    }

    if (target == "platforms") {
        std::set<std::string> platforms;
        for (const ModuleDescriptor& module : modules_) {
            for (const std::string& platform : module.supported_platforms) {
                platforms.insert(platform);
            }
        }

        for (const std::string& platform : platforms) {
            std::cout << "  " << platform << '\n';
        }
        return true;
    }

    if (target == "options") {
        std::cout << "Active module: " << (active_module_.empty() ? "<none>" : active_module_) << '\n';

        if (!active_module_.empty()) {
            const ModuleDescriptor descriptor = module_loader_.find_by_id(modules_, active_module_);
            std::cout << "Required and optional module options:\n";
            if (descriptor.options.empty()) {
                std::cout << "  <no declared manifest options>\n";
            } else {
                for (const ModuleOption& option : descriptor.options) {
                    const std::string key = to_upper(option.name);
                    std::string current = get_option_value(active_options_, global_options_, key);
                    if (current.empty()) {
                        current = option.default_value;
                    }

                    std::cout << "  " << key
                              << "  required=" << (option.required ? "yes" : "no")
                              << "  current=" << (current.empty() ? "<unset>" : current);
                    if (!option.description.empty()) {
                        std::cout << "  # " << option.description;
                    }
                    std::cout << '\n';
                }
            }
        } else {
            std::cout << "Module options:\n";
            if (active_options_.empty()) {
                std::cout << "  <none>\n";
            } else {
                for (const auto& [key, value] : active_options_) {
                    std::cout << "  " << key << " = " << value << '\n';
                }
            }
        }

        std::cout << "Global options:\n";
        if (global_options_.empty()) {
            std::cout << "  <none>\n";
        } else {
            for (const auto& [key, value] : global_options_) {
                std::cout << "  " << key << " = " << value << '\n';
            }
        }
        return true;
    }

    if (target == "sessions") {
        const auto sessions = session_manager_.list();
        if (sessions.empty()) {
            std::cout << "No active sessions.\n";
            return true;
        }

        for (const SessionInfo& session : sessions) {
            std::cout << "  " << session.session_id << "  " << session.host_label
                      << "  " << session.ip_version
                      << "  " << session.platform << "  " << session.transport_used
                      << "  " << session.local_bind << '\n';
        }
        return true;
    }

    const std::unordered_map<std::string, std::string> category_map = {
        {"payloads", "payload"},
        {"exploits", "exploit"},
        {"auxiliary", "auxiliary"},
        {"encoders", "encoder"},
        {"evasion", "evasion"},
        {"nops", "nop"},
        {"transports", "transport"},
        {"detection", "detection"},
        {"analysis", "analysis"},
        {"lab", "lab"}
    };

    const auto found = category_map.find(target);
    if (found == category_map.end()) {
        std::cout << "[-] Unsupported show target: " << target << '\n';
        return false;
    }

    std::vector<ModuleDescriptor> filtered;
    for (const ModuleDescriptor& module : modules_) {
        if (module.category == found->second) {
            filtered.push_back(module);
        }
    }

    print_module_table(filtered);
    return true;
}

bool ConsoleEngine::handle_sessions(const std::vector<std::string>& tokens) {
    if (tokens.size() == 1 || (tokens.size() == 2 && to_lower(tokens[1]) == "-l")) {
        return handle_show({"show", "sessions"});
    }

    const std::string action = to_lower(tokens[1]);

    if (tokens[1] == "-K" || action == "-kall") {
        session_manager_.clear();
        log_session_line("session_killed_all");
        std::cout << "[*] All sessions closed.\n";
        return true;
    }

    if (action == "-i") {
        if (tokens.size() < 3) {
            std::cout << "Usage: sessions -i <id>\n";
            return true;
        }
        return enter_pixelpreter(tokens[2]);
    }

    if (action == "-k") {
        if (tokens.size() < 3) {
            std::cout << "Usage: sessions -k <id>\n";
            return true;
        }
        if (session_manager_.remove(tokens[2])) {
            log_session_line("session_killed id=" + tokens[2]);
            std::cout << "[*] Session " << tokens[2] << " closed.\n";
        } else {
            std::cout << "[-] Session not found: " << tokens[2] << '\n';
        }
        return true;
    }

    std::cout << "Usage: sessions [-l] | sessions -i <id> | sessions -k <id> | sessions -K\n";
    return true;
}

bool ConsoleEngine::handle_log(const std::vector<std::string>& tokens) {
    if (tokens.size() < 2) {
        std::cout << "Usage: log <show|export> [file]\n";
        return true;
    }

    const std::string action = to_lower(tokens[1]);
    if (action == "show") {
        const std::string content = read_text_file(runtime_logs_dir() / "framework.log");
        if (content.empty()) {
            std::cout << "No framework log entries.\n";
        } else {
            std::cout << content;
        }
        return true;
    }

    if (action == "export") {
        if (tokens.size() < 3) {
            std::cout << "Usage: log export <file>\n";
            return true;
        }

        const std::string file_path = tokens[2];
        std::ofstream output(file_path, std::ios::trunc);
        if (!output) {
            std::cout << "[-] Unable to write log export.\n";
            return true;
        }

        output << "[framework.log]\n" << read_text_file(runtime_logs_dir() / "framework.log") << '\n';
        output << "[sessions.log]\n" << read_text_file(runtime_logs_dir() / "sessions.log") << '\n';
        output << "[errors.log]\n" << read_text_file(runtime_logs_dir() / "errors.log") << '\n';

        std::cout << "[+] Logs exported to " << file_path << '\n';
        return true;
    }

    std::cout << "[-] Unsupported log action.\n";
    return false;
}

bool ConsoleEngine::handle_report(const std::vector<std::string>& tokens) {
    if (tokens.size() < 2) {
        std::cout << "Usage: report <generate|export> [file]\n";
        return true;
    }

    const std::string action = to_lower(tokens[1]);
    if (action == "generate") {
        const std::filesystem::path report_path = runtime_workspace_reports_dir(active_workspace_) / "latest_report.txt";
        std::ofstream output(report_path, std::ios::trunc);
        if (!output) {
            std::cout << "[-] Unable to generate report.\n";
            return true;
        }

        output << "PixelFerrite Report\n";
        output << "Generated: " << now_string("%Y-%m-%d %H:%M:%S") << '\n';
        output << "Active workspace: " << active_workspace_ << '\n';
        output << "Module count: " << modules_.size() << '\n';
        output << "Session count: " << session_manager_.list().size() << '\n';
        output << "Active module: " << (active_module_.empty() ? "<none>" : active_module_) << '\n';

        std::cout << "[+] Report generated: " << report_path.generic_string() << '\n';
        return true;
    }

    if (action == "export") {
        if (tokens.size() < 3) {
            std::cout << "Usage: report export <file>\n";
            return true;
        }

        const std::string source = read_text_file(runtime_workspace_reports_dir(active_workspace_) / "latest_report.txt");
        if (source.empty()) {
            std::cout << "[-] No report available. Run 'report generate' first.\n";
            return true;
        }

        std::ofstream output(tokens[2], std::ios::trunc);
        if (!output) {
            std::cout << "[-] Unable to write report export.\n";
            return true;
        }
        output << source;
        std::cout << "[+] Report exported to " << tokens[2] << '\n';
        return true;
    }

    std::cout << "[-] Unsupported report action.\n";
    return false;
}

bool ConsoleEngine::handle_dataset(const std::vector<std::string>& tokens) {
    if (tokens.size() < 2) {
        std::cout << "Usage: dataset <generate|analyze|compare> [args]\n";
        return true;
    }

    const std::string action = to_lower(tokens[1]);
    if (action == "generate") {
        int count = 10;
        if (tokens.size() >= 3) {
            try {
                count = std::max(1, std::stoi(tokens[2]));
            } catch (...) {
                count = 10;
            }
        }

        const std::filesystem::path out_dir = runtime_workspace_dataset_dir(active_workspace_) / "generated";
        std::filesystem::create_directories(out_dir);

        for (int i = 1; i <= count; ++i) {
            std::ostringstream file_name;
            file_name << "sample_" << std::setw(4) << std::setfill('0') << i << ".json";

            std::ofstream output(out_dir / file_name.str(), std::ios::trunc);
            output << "{\n"
                   << "  \"id\": \"sample_" << std::setw(4) << std::setfill('0') << i << "\",\n"
                   << "  \"label\": \"image_simulation\",\n"
                   << "  \"source\": \"dataset_generate\"\n"
                   << "}\n";
        }

        std::cout << "[+] Generated " << count << " dataset entries in " << out_dir.generic_string() << '\n';
        return true;
    }

    if (action == "analyze") {
        const std::filesystem::path input_dir =
            (tokens.size() >= 3)
                ? std::filesystem::path(tokens[2])
                : runtime_workspace_dataset_dir(active_workspace_) / "generated";
        if (!std::filesystem::exists(input_dir)) {
            std::cout << "[-] Directory does not exist: " << input_dir.generic_string() << '\n';
            return true;
        }

        std::size_t count = 0;
        std::uintmax_t total_bytes = 0;
        for (const auto& entry : std::filesystem::directory_iterator(input_dir)) {
            if (!entry.is_regular_file()) {
                continue;
            }
            ++count;
            total_bytes += entry.file_size();
        }

        std::cout << "[*] Dataset analyze: files=" << count << ", bytes=" << total_bytes << '\n';
        return true;
    }

    if (action == "compare") {
        const std::filesystem::path left =
            (tokens.size() >= 3)
                ? std::filesystem::path(tokens[2])
                : runtime_workspace_dataset_dir(active_workspace_);
        const std::filesystem::path right =
            (tokens.size() >= 4)
                ? std::filesystem::path(tokens[3])
                : runtime_workspace_dataset_dir(active_workspace_) / "generated";

        auto file_count = [](const std::filesystem::path& path) -> std::size_t {
            if (!std::filesystem::exists(path)) {
                return 0;
            }
            std::size_t count = 0;
            for (const auto& entry : std::filesystem::directory_iterator(path)) {
                if (entry.is_regular_file()) {
                    ++count;
                }
            }
            return count;
        };

        const std::size_t left_count = file_count(left);
        const std::size_t right_count = file_count(right);
        std::cout << "[*] compare: " << left.generic_string() << "=" << left_count
                  << ", " << right.generic_string() << "=" << right_count << '\n';
        return true;
    }

    std::cout << "[-] Unsupported dataset action.\n";
    return false;
}

bool ConsoleEngine::handle_jobs(const std::vector<std::string>& tokens) {
    if (tokens.size() == 1 || (tokens.size() >= 2 && tokens[1] == "-l")) {
        if (jobs_.empty()) {
            std::cout << "[*] No jobs recorded.\n";
            return true;
        }
        std::cout << "ID  STATUS     TYPE          STARTED              DETAIL\n";
        for (const JobRecord& job : jobs_) {
            std::cout << std::setw(2) << job.id << "  "
                      << std::setw(10) << job.status << "  "
                      << std::setw(12) << job.kind << "  "
                      << std::setw(19) << job.started_at << "  "
                      << job.detail << '\n';
        }
        return true;
    }

    if (tokens.size() >= 2 && tokens[1] == "-k") {
        if (tokens.size() < 3) {
            std::cout << "Usage: jobs -k <id>\n";
            return true;
        }
        for (JobRecord& job : jobs_) {
            if (job.id == tokens[2]) {
                if (job.status == "running") {
                    job.status = "stopped";
                    job.completed_at = now_string("%Y-%m-%d %H:%M:%S");
                    std::cout << "[*] Job stopped: " << job.id << '\n';
                } else {
                    std::cout << "[-] Job is not running: " << job.id << '\n';
                }
                return true;
            }
        }
        std::cout << "[-] Job not found: " << tokens[2] << '\n';
        return true;
    }

    if (tokens.size() >= 2 && tokens[1] == "-K") {
        std::size_t stopped = 0;
        for (JobRecord& job : jobs_) {
            if (job.status == "running") {
                job.status = "stopped";
                job.completed_at = now_string("%Y-%m-%d %H:%M:%S");
                ++stopped;
            }
        }
        std::cout << "[*] Stopped running jobs: " << stopped << '\n';
        return true;
    }

    std::cout << "Usage: jobs [-l] | jobs -k <id> | jobs -K\n";
    return true;
}

bool ConsoleEngine::handle_resource(const std::vector<std::string>& tokens) {
    if (tokens.size() < 2) {
        std::cout << "Usage: resource <file>\n";
        return true;
    }
    if (resource_depth_ >= 3) {
        std::cout << "[-] resource nesting limit reached.\n";
        return true;
    }

    const std::filesystem::path resource_file = tokens[1];
    std::ifstream input(resource_file);
    if (!input) {
        std::cout << "[-] Unable to open resource file: " << resource_file.generic_string() << '\n';
        return true;
    }

    ++resource_depth_;
    std::string line;
    std::size_t executed = 0;
    std::size_t skipped = 0;
    while (std::getline(input, line)) {
        const std::string trimmed = trim(line);
        if (trimmed.empty() || trimmed.rfind("#", 0) == 0) {
            ++skipped;
            continue;
        }
        std::cout << "[resource] " << trimmed << '\n';
        handle_command(trimmed);
        ++executed;
    }
    --resource_depth_;

    std::cout << "[*] resource complete: executed=" << executed << ", skipped=" << skipped << '\n';
    return true;
}

bool ConsoleEngine::handle_makerc(const std::vector<std::string>& tokens) {
    const std::filesystem::path out_file =
        (tokens.size() >= 2)
            ? std::filesystem::path(tokens[1])
            : runtime_workspace_scripts_dir(active_workspace_) / "pffconsole.rc";

    std::filesystem::create_directories(out_file.parent_path());
    std::ofstream output(out_file, std::ios::trunc);
    if (!output) {
        std::cout << "[-] Unable to write rc file: " << out_file.generic_string() << '\n';
        return true;
    }

    for (const std::string& cmd : history_) {
        output << cmd << '\n';
    }
    std::cout << "[+] rc script written: " << out_file.generic_string() << '\n';
    return true;
}

bool ConsoleEngine::handle_save(const std::vector<std::string>& tokens) {
    const std::filesystem::path out_file =
        (tokens.size() >= 2)
            ? std::filesystem::path(tokens[1])
            : runtime_workspace_state_dir(active_workspace_) / "console_state.txt";

    std::filesystem::create_directories(out_file.parent_path());
    std::ofstream output(out_file, std::ios::trunc);
    if (!output) {
        std::cout << "[-] Unable to write state file: " << out_file.generic_string() << '\n';
        return true;
    }

    output << "workspace=" << active_workspace_ << '\n';
    output << "module=" << (active_module_.empty() ? "<none>" : active_module_) << '\n';
    output << "platform=" << active_platform_ << '\n';
    output << "scope_enforce=" << (enforce_scope_ ? "on" : "off") << '\n';
    output << "threads=" << max_threads_ << '\n';
    output << "db_connected=" << (db_connected_ ? "yes" : "no") << '\n';
    output << "db_target=" << db_target_ << '\n';
    output << "[global_options]\n";
    for (const auto& [key, value] : global_options_) {
        output << key << "=" << value << '\n';
    }
    output << "[module_options]\n";
    for (const auto& [key, value] : active_options_) {
        output << key << "=" << value << '\n';
    }
    output << "[scope]\n";
    for (const std::string& target : allowed_targets_) {
        output << target << '\n';
    }

    std::cout << "[+] console state saved: " << out_file.generic_string() << '\n';
    return true;
}

bool ConsoleEngine::handle_spool(const std::vector<std::string>& tokens) {
    if (tokens.size() < 2 || to_lower(tokens[1]) == "status") {
        std::cout << "spool=" << (spool_enabled_ ? "on" : "off");
        if (spool_enabled_) {
            std::cout << " file=" << spool_file_.generic_string();
        }
        std::cout << '\n';
        std::cout << "Usage: spool <start [file]|stop|status>\n";
        return true;
    }

    const std::string action = to_lower(tokens[1]);
    if (action == "start") {
        spool_file_ = (tokens.size() >= 3)
                          ? std::filesystem::path(tokens[2])
                          : runtime_workspace_logs_dir(active_workspace_) /
                                ("spool_" + compact_timestamp() + ".log");
        std::filesystem::create_directories(spool_file_.parent_path());
        spool_enabled_ = true;
        append_spool_line("# spool started " + now_string("%Y-%m-%d %H:%M:%S"));
        std::cout << "[+] spool started: " << spool_file_.generic_string() << '\n';
        return true;
    }

    if (action == "stop") {
        if (!spool_enabled_) {
            std::cout << "[-] spool is not active.\n";
            return true;
        }
        append_spool_line("# spool stopped " + now_string("%Y-%m-%d %H:%M:%S"));
        spool_enabled_ = false;
        std::cout << "[*] spool stopped.\n";
        return true;
    }

    std::cout << "Usage: spool <start [file]|stop|status>\n";
    return true;
}

bool ConsoleEngine::handle_threads(const std::vector<std::string>& tokens) {
    if (tokens.size() < 2) {
        std::cout << "[*] threads => " << max_threads_ << '\n';
        std::cout << "Usage: threads <count>\n";
        return true;
    }

    try {
        const int requested = std::stoi(tokens[1]);
        max_threads_ = std::clamp(requested, 1, 64);
        std::cout << "[*] threads set to " << max_threads_ << '\n';
    } catch (...) {
        std::cout << "[-] Invalid thread count.\n";
    }
    return true;
}

bool ConsoleEngine::handle_route(const std::vector<std::string>& tokens) {
    if (tokens.size() < 2 || to_lower(tokens[1]) == "list" || to_lower(tokens[1]) == "print") {
        if (routes_.empty()) {
            std::cout << "[*] No routes configured.\n";
            return true;
        }
        std::cout << "Subnet                Via\n";
        for (const RouteEntry& route : routes_) {
            std::cout << std::setw(20) << route.subnet << "  " << route.via << '\n';
        }
        return true;
    }

    const std::string action = to_lower(tokens[1]);
    if (action == "add") {
        if (tokens.size() < 4) {
            std::cout << "Usage: route add <subnet/cidr> <gateway>\n";
            return true;
        }
        routes_.push_back(RouteEntry{tokens[2], tokens[3]});
        std::cout << "[+] route added: " << tokens[2] << " via " << tokens[3] << '\n';
        return true;
    }

    if (action == "del" || action == "delete" || action == "remove") {
        if (tokens.size() < 3) {
            std::cout << "Usage: route del <subnet/cidr>\n";
            return true;
        }
        const std::size_t old_size = routes_.size();
        routes_.erase(
            std::remove_if(routes_.begin(), routes_.end(), [&](const RouteEntry& route) {
                return route.subnet == tokens[2];
            }),
            routes_.end()
        );
        std::cout << (routes_.size() != old_size ? "[*] route removed.\n" : "[-] route not found.\n");
        return true;
    }

    if (action == "flush") {
        routes_.clear();
        std::cout << "[*] all routes removed.\n";
        return true;
    }

    std::cout << "Usage: route <list|add|del|flush>\n";
    return true;
}

bool ConsoleEngine::handle_connect(const std::vector<std::string>& tokens) {
    if (tokens.size() < 3) {
        std::cout << "Usage: connect <host> <port>\n";
        return true;
    }

    const std::string host = tokens[1];
    int port = 0;
    try {
        port = std::stoi(tokens[2]);
    } catch (...) {
        std::cout << "[-] Invalid port: " << tokens[2] << '\n';
        return true;
    }
    if (port < 1 || port > 65535) {
        std::cout << "[-] Port out of range: " << port << '\n';
        return true;
    }

    if (enforce_scope_ && !is_target_in_scope(allowed_targets_, host)) {
        std::cout << "[-] blocked by scope policy: " << host << '\n';
        return true;
    }

    std::cout << "[*] simulated connect => " << host << ":" << port << " (ok)\n";
    return true;
}

bool ConsoleEngine::handle_db_status() const {
    std::cout << "database: " << (db_connected_ ? "connected" : "disconnected")
              << " target=" << db_target_ << '\n';
    return true;
}

bool ConsoleEngine::handle_db_connect(const std::vector<std::string>& tokens) {
    if (tokens.size() < 2) {
        std::cout << "Usage: db_connect <name|dsn>\n";
        return true;
    }
    db_target_ = join(tokens, 1);
    db_connected_ = true;
    std::cout << "[+] database connected: " << db_target_ << " (simulation)\n";
    return true;
}

bool ConsoleEngine::handle_set(const std::vector<std::string>& tokens) {
    if (tokens.size() < 3) {
        std::cout << "Usage: set <key> <value>\n";
        return true;
    }

    const std::string key = to_upper(tokens[1]);
    const std::string value = join(tokens, 2);

    if (key == "PLATFORM") {
        active_platform_ = to_lower(value);
        std::cout << "platform => " << active_platform_ << '\n';
        return true;
    }

    active_options_[key] = value;
    std::cout << key << " => " << value << '\n';
    return true;
}

bool ConsoleEngine::handle_unset(const std::vector<std::string>& tokens) {
    if (tokens.size() < 2) {
        std::cout << "Usage: unset <key|all>\n";
        return true;
    }

    if (to_lower(tokens[1]) == "all") {
        active_options_.clear();
        std::cout << "[*] Cleared module options.\n";
        return true;
    }

    const std::size_t erased = active_options_.erase(to_upper(tokens[1]));
    std::cout << (erased ? "[*] Option removed.\n" : "[-] Option not set.\n");
    return true;
}

bool ConsoleEngine::handle_use(const std::vector<std::string>& tokens) {
    if (tokens.size() < 2) {
        std::cout << "Usage: use <module_id>\n";
        return true;
    }

    const std::string module_id = normalize_module_id(tokens[1]);
    const ModuleDescriptor module = module_loader_.find_by_id(modules_, module_id);
    if (module.id.empty()) {
        std::cout << "[-] Module not found: " << module_id << '\n';
        return true;
    }

    active_module_ = module.id;
    active_options_.clear();
    std::cout << "[*] Using module " << active_module_ << '\n';
    return true;
}

bool ConsoleEngine::handle_search(const std::vector<std::string>& tokens) const {
    if (tokens.size() < 2) {
        std::cout << "Usage: search <keyword>\n";
        return true;
    }

    const std::string keyword = to_lower(join(tokens, 1));
    std::vector<ModuleDescriptor> matches;

    for (const ModuleDescriptor& module : modules_) {
        const std::string haystack = to_lower(module.id + " " + module.name + " " + module.category);
        if (haystack.find(keyword) != std::string::npos) {
            matches.push_back(module);
        }
    }

    std::cout << "[*] Search results: " << matches.size() << '\n';
    print_module_table(matches);
    return true;
}

bool ConsoleEngine::handle_info() const {
    if (active_module_.empty()) {
        std::cout << "[-] No active module. Use 'use <module_id>' first.\n";
        return true;
    }

    const ModuleDescriptor module = module_loader_.find_by_id(modules_, active_module_);
    if (module.id.empty()) {
        std::cout << "[-] Active module manifest unavailable.\n";
        return true;
    }

    std::cout << "Name: " << module.name << '\n';
    std::cout << "ID: " << module.id << '\n';
    std::cout << "Category: " << module.category << '\n';
    std::cout << "Path: " << module.path << '\n';
    std::cout << "Author: " << (module.author.empty() ? "n/a" : module.author) << '\n';
    std::cout << "Version: " << (module.version.empty() ? "n/a" : module.version) << '\n';
    std::cout << "Safety: " << (module.safety_level.empty() ? "n/a" : module.safety_level) << '\n';

    std::cout << "Supported platforms: ";
    if (module.supported_platforms.empty()) {
        std::cout << "n/a\n";
    } else {
        for (std::size_t i = 0; i < module.supported_platforms.size(); ++i) {
            if (i > 0) {
                std::cout << ", ";
            }
            std::cout << module.supported_platforms[i];
        }
        std::cout << '\n';
    }

    std::cout << "Supported arch: ";
    if (module.supported_arch.empty()) {
        std::cout << "n/a\n";
    } else {
        for (std::size_t i = 0; i < module.supported_arch.size(); ++i) {
            if (i > 0) {
                std::cout << ", ";
            }
            std::cout << module.supported_arch[i];
        }
        std::cout << '\n';
    }
    return true;
}

bool ConsoleEngine::handle_run_like(const std::string& action) {
    if (active_module_.empty()) {
        std::cout << "[-] No active module selected.\n";
        return true;
    }

    std::cout << "[*] " << action << " => " << active_module_ << '\n';
    const ModuleDescriptor active_descriptor = module_loader_.find_by_id(modules_, active_module_);
    const std::vector<std::string> missing_required =
        find_missing_required_options(active_descriptor, active_options_, global_options_);

    if (action == "check") {
        const std::string rhosts = get_option_value(active_options_, global_options_, "RHOSTS");
        const std::string rhost = get_option_value(active_options_, global_options_, "RHOST");
        const std::string rhost6 = get_option_value(active_options_, global_options_, "RHOST6");

        std::vector<std::string> targets;
        if (!rhosts.empty()) {
            targets = split_targets(rhosts);
        } else if (!rhost.empty()) {
            targets.push_back(rhost);
        } else if (!rhost6.empty()) {
            targets.push_back(rhost6);
        } else {
            targets.push_back("lab-node-auto");
        }

        std::cout << "[*] module category: " << active_descriptor.category << '\n';
        std::cout << "[*] targets provided: " << targets.size() << '\n';
        std::cout << "[*] scope enforcement: " << (enforce_scope_ ? "on" : "off") << '\n';
        if (missing_required.empty()) {
            std::cout << "[*] required options: satisfied\n";
        } else {
            std::cout << "[-] missing required options:\n";
            for (const std::string& option : missing_required) {
                std::cout << "  " << option << '\n';
            }
        }

        std::size_t in_scope = 0;
        for (const std::string& target : targets) {
            const bool allowed = !enforce_scope_ || is_target_in_scope(allowed_targets_, target);
            std::cout << "  " << target << " => " << (allowed ? "allowed" : "blocked") << '\n';
            if (allowed) {
                ++in_scope;
            }
        }

        if (in_scope > 0 && missing_required.empty()) {
            std::cout << "[+] ready to run\n";
        } else if (in_scope == 0) {
            std::cout << "[-] no in-scope targets\n";
        }
        return true;
    }

    if (action == "simulate" || action == "run" || action == "exploit") {
        if (!missing_required.empty()) {
            std::cout << "[-] missing required options:\n";
            for (const std::string& option : missing_required) {
                std::cout << "  " << option << '\n';
            }
            std::cout << "Use 'show options' and set required values before run.\n";
            return true;
        }

        const std::string transport = [&] {
            const std::string configured = get_option_value(active_options_, global_options_, "TRANSPORT");
            return configured.empty() ? "http_simulated" : configured;
        }();

        std::vector<std::string> targets;
        const std::string rhosts = get_option_value(active_options_, global_options_, "RHOSTS");
        const std::string rhost = get_option_value(active_options_, global_options_, "RHOST");
        const std::string rhost6 = get_option_value(active_options_, global_options_, "RHOST6");

        if (!rhosts.empty()) {
            targets = split_targets(rhosts);
        } else if (!rhost.empty()) {
            targets.push_back(rhost);
        } else if (!rhost6.empty()) {
            targets.push_back(rhost6);
        } else {
            targets.push_back("lab-node-auto");
        }

        const std::string lhost = get_option_value(active_options_, global_options_, "LHOST");
        const std::string lhost6 = get_option_value(active_options_, global_options_, "LHOST6");
        const std::string lport = get_option_value(active_options_, global_options_, "LPORT");

        std::vector<std::string> effective_targets;
        for (const std::string& target : targets) {
            if (enforce_scope_ && !is_target_in_scope(allowed_targets_, target)) {
                std::cout << "[-] blocked by scope policy: " << target << '\n';
                continue;
            }
            effective_targets.push_back(target);
        }

        if (effective_targets.empty()) {
            std::cout << "[-] no eligible targets after scope checks.\n";
            return true;
        }

        if (active_descriptor.category == "auxiliary") {
            std::string report_body;

            if (active_module_ == "auxiliary/device_info/local_system_inventory") {
                report_body = run_local_device_inventory();
            } else if (active_module_ == "auxiliary/device_info/cpu_memory_profile") {
                report_body = run_local_cpu_memory_profile();
            } else if (active_module_ == "auxiliary/logs/local_log_snapshot") {
                report_body = run_local_log_snapshot();
            } else if (active_module_ == "auxiliary/vm/hypervisor_inventory") {
                report_body = run_hypervisor_inventory();
            } else if (active_module_ == "auxiliary/network/reachability_probe") {
                report_body = run_ping_probe(effective_targets);
            } else if (active_module_ == "auxiliary/network/dualstack_connectivity_audit") {
                report_body = run_ping_probe(effective_targets);
            } else {
                report_body = "module=" + active_module_ + "\nstatus=executed\n";
            }

            const std::filesystem::path report_path =
                runtime_workspace_reports_dir(active_workspace_) /
                (short_module_name(active_module_) + "_" + compact_timestamp() + ".txt");

            std::ofstream report_output(report_path, std::ios::trunc);
            if (report_output) {
                report_output << "module=" << active_module_ << '\n';
                report_output << "action=" << action << '\n';
                report_output << "targets=" << join(effective_targets, 0) << '\n';
                report_output << "transport=" << transport << '\n';
                report_output << "timestamp=" << now_string("%Y-%m-%d %H:%M:%S") << "\n\n";
                report_output << report_body << '\n';
            }

            std::cout << "[+] auxiliary module completed. report: " << report_path.generic_string() << '\n';
            log_session_line("aux_run module=" + active_module_ + " report=" + report_path.generic_string());
            return true;
        }

        for (const std::string& target : effective_targets) {
            SessionInfo session;
            session.session_id = session_manager_.next_id();
            session.target_endpoint = target;
            session.host_label = target;
            session.ip_version = detect_ip_version(target);
            session.platform = active_platform_;
            session.transport_used = transport;

            if (session.ip_version == "ipv6") {
                session.local_bind = lhost6.empty() ? "::" : lhost6;
            } else {
                session.local_bind = lhost.empty() ? "0.0.0.0" : lhost;
            }
            if (!lport.empty()) {
                if (session.ip_version == "ipv6") {
                    session.local_bind = "[" + session.local_bind + "]:" + lport;
                } else {
                    session.local_bind += ":" + lport;
                }
            }

            session_manager_.add(session);

            const std::filesystem::path session_path =
                runtime_workspace_sessions_dir(active_workspace_) / ("session_" + session.session_id + ".json");
            std::ofstream output(session_path, std::ios::trunc);
            if (output) {
                output << "{\n"
                       << "  \"session_id\": \"" << session.session_id << "\",\n"
                       << "  \"host_label\": \"" << session.host_label << "\",\n"
                       << "  \"target_endpoint\": \"" << session.target_endpoint << "\",\n"
                       << "  \"ip_version\": \"" << session.ip_version << "\",\n"
                       << "  \"platform\": \"" << session.platform << "\",\n"
                       << "  \"transport_used\": \"" << session.transport_used << "\",\n"
                       << "  \"local_bind\": \"" << session.local_bind << "\",\n"
                       << "  \"agent\": \"pixelpreter\"\n"
                       << "}\n";
            }

            log_session_line(
                "session_open id=" + session.session_id +
                " module=" + active_module_ +
                " target=" + session.target_endpoint +
                " ip=" + session.ip_version +
                " platform=" + session.platform
            );

            std::cout << "[+] pixelpreter session " << session.session_id
                      << " opened target=" << session.target_endpoint
                      << " " << session.ip_version
                      << " via " << session.transport_used << '\n';
        }
    }

    return true;
}

bool ConsoleEngine::enter_pixelpreter(const std::string& session_id) {
    const SessionInfo session = session_manager_.find(session_id);
    if (session.session_id.empty()) {
        std::cout << "[-] Session not found: " << session_id << '\n';
        return true;
    }

    std::cout << "[*] Interacting with pixelpreter session " << session.session_id << '\n';
    std::cout << "Type 'help' for session commands, 'background' to return.\n";

    std::string line;
    while (
        std::cout << colors::wrap("pixelpreter(" + session.session_id + ")>", colors::prompt()) << " " &&
        std::getline(std::cin, line)
    ) {
        const std::vector<std::string> tokens = tokenize(line);
        if (tokens.empty()) {
            continue;
        }

        const std::string cmd = to_lower(tokens[0]);
        if (cmd == "background" || cmd == "exit" || cmd == "quit") {
            std::cout << "[*] Backgrounding session " << session.session_id << '\n';
            return true;
        }
        if (cmd == "help" || cmd == "?") {
            std::cout
                << "pixelpreter commands:\n"
                << "  help, background, sysinfo, getuid, ps, ifconfig, pwd, ls,\n"
                << "  download <remote> <local>, upload <local> <remote>\n";
            continue;
        }
        if (cmd == "sysinfo") {
            std::cout << "Computer: " << session.host_label << '\n';
            std::cout << "OS: " << session.platform << " (simulated)\n";
            std::cout << "Agent: pixelpreter/0.1.0\n";
            std::cout << "Target: " << session.target_endpoint << " [" << session.ip_version << "]\n";
            std::cout << "Listener: " << session.local_bind << '\n';
            continue;
        }
        if (cmd == "getuid") {
            std::cout << "Server username: lab\\\\researcher\n";
            continue;
        }
        if (cmd == "ps") {
            std::cout
                << " PID  NAME\n"
                << " 100  pixelpreter_host\n"
                << " 200  telemetry_stub\n"
                << " 300  image_pipeline\n";
            continue;
        }
        if (cmd == "ifconfig") {
            if (session.ip_version == "ipv6") {
                std::cout << "eth0 fd13:37::" << session.session_id << "/64\n";
            } else {
                std::cout << "eth0 10.13.37." << session.session_id << "/24\n";
            }
            continue;
        }
        if (cmd == "pwd") {
            std::cout << "/lab/simulated/session_" << session.session_id << '\n';
            continue;
        }
        if (cmd == "ls") {
            std::cout << "logs/  datasets/  artifacts/\n";
            continue;
        }
        if (cmd == "download") {
            std::cout << "[*] Simulated download complete.\n";
            continue;
        }
        if (cmd == "upload") {
            std::cout << "[*] Simulated upload complete.\n";
            continue;
        }

        std::cout << "[-] Unknown pixelpreter command: " << cmd << '\n';
    }

    return true;
}

std::string ConsoleEngine::start_job(const std::string& kind, const std::string& detail) {
    JobRecord job;
    job.id = std::to_string(next_job_id_++);
    job.kind = kind;
    job.detail = detail;
    job.status = "running";
    job.started_at = now_string("%Y-%m-%d %H:%M:%S");
    jobs_.push_back(job);
    return job.id;
}

void ConsoleEngine::finish_job(const std::string& job_id, const std::string& status) {
    for (auto it = jobs_.rbegin(); it != jobs_.rend(); ++it) {
        if (it->id == job_id) {
            it->status = status;
            it->completed_at = now_string("%Y-%m-%d %H:%M:%S");
            return;
        }
    }
}

void ConsoleEngine::append_spool_line(const std::string& line) const {
    if (!spool_enabled_ || spool_file_.empty()) {
        return;
    }
    std::ofstream output(spool_file_, std::ios::app);
    if (output) {
        output << line << '\n';
    }
}

std::string ConsoleEngine::prompt() const {
    return colors::wrap("pff>", colors::prompt()) + " ";
}

void ConsoleEngine::print_help() const {
    std::cout
        << hdr("Core:") << '\n'
        << "  help, banner, version, clear, history, exit, quit\n"
        << hdr("Workspace:") << '\n'
        << "  workspace list|add <name>|select <name>|delete <name>\n"
        << hdr("Tools policy:") << '\n'
        << "  tools list, tools allow <tool>, tools deny <tool>, tools reset\n"
        << hdr("Scope:") << '\n'
        << "  scope list, scope add <target>, scope remove <target>, scope clear,\n"
        << "  scope enforce <on|off>\n"
        << hdr("Discovery:") << '\n'
        << "  search <keyword>\n"
        << "  show modules|payloads|exploits|auxiliary|encoders|evasion|nops|transports|detection|analysis|lab|platforms|options|sessions\n"
        << hdr("Module lifecycle:") << '\n'
        << "  use <module_id>, info, back, set <k> <v>, unset <k|all>, setg <k> <v>, unsetg <k|all>\n"
        << hdr("Execution workflow:") << '\n'
        << "  run, exploit, check, embed, extract, mutate, simulate, analyze\n"
        << "  network opts: set RHOST <ipv4>, set RHOST6 <ipv6>, set RHOSTS <list>,\n"
        << "                set LHOST <ipv4>, set LHOST6 <ipv6>, set LPORT <port>\n"
        << hdr("Session management:") << '\n'
        << "  sessions, sessions -l, sessions -i <id>, sessions -k <id>, sessions -K\n"
        << hdr("Logs and reports:") << '\n'
        << "  log show, log export <file>, report generate, report export <file>\n"
        << hdr("Dataset:") << '\n'
        << "  dataset generate <n>, dataset analyze <dir>, dataset compare <left> <right>\n"
        << hdr("External tools:") << '\n'
        << "  nmap <args>, netprobe-rs <args> (if installed in PATH)\n"
        << hdr("Automation and Ops:") << '\n'
        << "  reload_all, jobs [-l|-k <id>|-K], threads <count>\n"
        << "  resource <file>, makerc [file], save [file], spool <start [file]|stop|status>\n"
        << "  route <list|add|del|flush>, connect <host> <port>, db_connect <name>, db_status\n"
        << hdr("Compatibility stubs:") << '\n'
        << "  load, unload, irb, pry, cd, color, debug, edit, hosts, services, creds,\n"
        << "  loot, notes, vulns\n";
}

std::string ConsoleEngine::trim(const std::string& value) {
    const std::size_t first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }
    const std::size_t last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

std::vector<std::string> ConsoleEngine::tokenize(const std::string& line) {
    std::vector<std::string> tokens;
    std::string token;
    bool in_quotes = false;

    for (char ch : line) {
        if (ch == '"') {
            in_quotes = !in_quotes;
            continue;
        }

        if (std::isspace(static_cast<unsigned char>(ch)) && !in_quotes) {
            if (!token.empty()) {
                tokens.push_back(token);
                token.clear();
            }
            continue;
        }

        token.push_back(ch);
    }

    if (!token.empty()) {
        tokens.push_back(token);
    }

    return tokens;
}

std::string ConsoleEngine::join(const std::vector<std::string>& tokens, std::size_t start_index) {
    if (start_index >= tokens.size()) {
        return {};
    }

    std::ostringstream joined;
    for (std::size_t i = start_index; i < tokens.size(); ++i) {
        if (i > start_index) {
            joined << ' ';
        }
        joined << tokens[i];
    }
    return joined.str();
}

}  // namespace pixelferrite

