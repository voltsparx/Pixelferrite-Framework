#pragma once

#include <string>
#include <vector>

namespace pixelferrite {

struct SessionInfo {
    std::string session_id;
    std::string host_label;
    std::string target_endpoint;
    std::string ip_version;
    std::string local_bind;
    std::string platform;
    std::string transport_used;
    std::string simulation_profile;
    int quality_score = 0;
    int detection_risk = 0;
};

class SessionManager {
public:
    void add(SessionInfo session);
    std::vector<SessionInfo> list() const;
    SessionInfo find(const std::string& session_id) const;
    bool remove(const std::string& session_id);
    void clear();
    std::string next_id();

private:
    unsigned int session_counter_ = 0;
    std::vector<SessionInfo> sessions_;
};

}  // namespace pixelferrite
