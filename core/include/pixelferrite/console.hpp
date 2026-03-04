#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <pixelferrite/module_loader.hpp>
#include <pixelferrite/session_manager.hpp>

namespace pixelferrite {

class ConsoleEngine {
public:
    explicit ConsoleEngine(std::string modules_root = "modules");

    void show_banner() const;
    void run_repl();

private:
    bool handle_command(const std::string& line);
    bool handle_workspace(const std::vector<std::string>& tokens);
    bool handle_tools(const std::vector<std::string>& tokens);
    bool handle_scope(const std::vector<std::string>& tokens);
    bool handle_explain(const std::vector<std::string>& tokens) const;
    bool handle_verify(const std::vector<std::string>& tokens) const;
    bool handle_show(const std::vector<std::string>& tokens) const;
    bool handle_sessions(const std::vector<std::string>& tokens);
    bool handle_log(const std::vector<std::string>& tokens);
    bool handle_report(const std::vector<std::string>& tokens);
    bool handle_dataset(const std::vector<std::string>& tokens);
    bool handle_jobs(const std::vector<std::string>& tokens);
    bool handle_resource(const std::vector<std::string>& tokens);
    bool handle_makerc(const std::vector<std::string>& tokens);
    bool handle_save(const std::vector<std::string>& tokens);
    bool handle_spool(const std::vector<std::string>& tokens);
    bool handle_threads(const std::vector<std::string>& tokens);
    bool handle_route(const std::vector<std::string>& tokens);
    bool handle_connect(const std::vector<std::string>& tokens);
    bool handle_db_status() const;
    bool handle_db_connect(const std::vector<std::string>& tokens);
    bool handle_set(const std::vector<std::string>& tokens);
    bool handle_unset(const std::vector<std::string>& tokens);
    bool handle_use(const std::vector<std::string>& tokens);
    bool handle_search(const std::vector<std::string>& tokens) const;
    bool handle_info() const;
    bool handle_run_like(const std::string& action);
    bool enter_pixelpreter(const std::string& session_id);
    std::string start_job(const std::string& kind, const std::string& detail);
    void finish_job(const std::string& job_id, const std::string& status);
    void append_spool_line(const std::string& line) const;
    std::string prompt() const;
    void print_help() const;

    static std::vector<std::string> tokenize(const std::string& line);
    static std::string join(const std::vector<std::string>& tokens, std::size_t start_index);
    static std::string trim(const std::string& value);

    struct JobRecord {
        std::string id;
        std::string kind;
        std::string detail;
        std::string status;
        std::string started_at;
        std::string completed_at;
    };

    struct RouteEntry {
        std::string subnet;
        std::string via;
    };

    std::string modules_root_;
    std::vector<ModuleDescriptor> modules_;
    SessionManager session_manager_;
    ModuleLoader module_loader_;

    std::vector<std::string> history_;
    std::vector<std::string> workspaces_ = {"default"};
    std::string active_workspace_ = "default";
    std::string active_platform_ = "cross_platform";
    std::string active_module_;
    std::unordered_map<std::string, std::string> active_options_;
    std::unordered_map<std::string, std::string> global_options_;
    std::unordered_map<std::string, std::unordered_set<std::string>> allowed_tools_by_workspace_;
    std::unordered_map<std::string, std::unordered_set<std::string>> denied_tools_by_workspace_;
    std::vector<std::string> allowed_targets_;
    std::vector<JobRecord> jobs_;
    std::vector<RouteEntry> routes_;
    std::filesystem::path config_root_;
    std::vector<std::string> logging_sinks_;
    std::string logging_level_ = "info";
    std::string framework_version_ = "0.1.0";
    std::string safety_mode_ = "simulation_only";
    std::string default_workspace_name_ = "default";
    int next_job_id_ = 1;
    int max_threads_ = 4;
    bool db_connected_ = false;
    std::string db_target_ = "none";
    bool spool_enabled_ = false;
    std::filesystem::path spool_file_;
    int resource_depth_ = 0;
    bool enforce_scope_ = false;
    bool running_ = true;
    std::string last_explain_summary_;
    std::vector<std::string> last_explain_lines_;
};

}  // namespace pixelferrite
