#include <pixelferrite/session_manager.hpp>

#include <algorithm>
#include <sstream>
#include <utility>

namespace pixelferrite {

void SessionManager::add(SessionInfo session) {
    sessions_.push_back(std::move(session));
}

std::vector<SessionInfo> SessionManager::list() const {
    return sessions_;
}

SessionInfo SessionManager::find(const std::string& session_id) const {
    const auto found = std::find_if(sessions_.begin(), sessions_.end(), [&](const SessionInfo& session) {
        return session.session_id == session_id;
    });

    return (found != sessions_.end()) ? *found : SessionInfo{};
}

bool SessionManager::remove(const std::string& session_id) {
    const auto old_size = sessions_.size();
    sessions_.erase(
        std::remove_if(sessions_.begin(), sessions_.end(), [&](const SessionInfo& session) {
            return session.session_id == session_id;
        }),
        sessions_.end()
    );

    return sessions_.size() != old_size;
}

void SessionManager::clear() {
    sessions_.clear();
}

std::string SessionManager::next_id() {
    ++session_counter_;
    std::ostringstream stream;
    stream << session_counter_;
    return stream.str();
}

}  // namespace pixelferrite
