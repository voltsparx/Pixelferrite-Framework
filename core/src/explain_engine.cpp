#include <pixelferrite/explain_engine.hpp>

#include <sstream>

namespace pixelferrite {

std::string ExplainEngine::summary(const ExplainRequest& request) const {
    std::ostringstream out;
    out << "module=" << request.descriptor.id
        << " profile=" << request.outcome.execution_profile
        << " confidence=" << request.outcome.confidence_score
        << " stability=" << request.outcome.overall_stability;
    return out.str();
}

std::vector<std::string> ExplainEngine::details(const ExplainRequest& request) const {
    std::vector<std::string> lines;

    lines.push_back(
        "strategy: category '" + request.descriptor.category + "' uses profile '" +
        request.outcome.execution_profile + "' for '" + request.plan.action + "'."
    );

    lines.push_back(
        "targeting: processed " + std::to_string(request.plan.targets.size()) +
        " target(s), ipv4=" + std::to_string(request.outcome.ipv4_targets) +
        ", ipv6=" + std::to_string(request.outcome.ipv6_targets) + "."
    );

    lines.push_back(
        "quality: confidence=" + std::to_string(request.outcome.confidence_score) +
        ", stability=" + std::to_string(request.outcome.overall_stability) +
        ", dualstack_ready=" + std::string(request.outcome.dual_stack_ready ? "yes" : "no") + "."
    );

    if (!request.missing_required.empty()) {
        std::ostringstream missing;
        missing << "readiness: missing required options";
        for (const std::string& key : request.missing_required) {
            missing << " " << key;
        }
        lines.push_back(missing.str());
    } else {
        lines.push_back("readiness: required options satisfied.");
    }

    lines.push_back(
        "policy: scope enforcement is " + std::string(request.scope_enforced ? "on" : "off") +
        ", transport='" + request.plan.transport + "'."
    );

    return lines;
}

}  // namespace pixelferrite

