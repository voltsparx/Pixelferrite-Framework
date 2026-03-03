#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include <pixelferrite/module_loader.hpp>

namespace pixelferrite {

struct SimulationTargetResult {
    std::string target;
    std::string ip_version;
    int latency_ms = 0;
    int jitter_ms = 0;
    int success_probability = 0;
    int stealth_score = 0;
    int detection_risk = 0;
    std::string quality_band;
};

struct SimulationPlan {
    std::string action;
    std::string module_id;
    std::string category;
    std::string platform;
    std::string transport;
    std::vector<std::string> targets;
    bool scope_enforced = false;
    int intensity = 70;
};

struct SimulationOutcome {
    std::string execution_profile;
    int confidence_score = 0;
    int overall_stability = 0;
    bool dual_stack_ready = false;
    std::size_t ipv4_targets = 0;
    std::size_t ipv6_targets = 0;
    std::vector<SimulationTargetResult> target_results;
    std::vector<std::string> timeline;
};

class SimulationEngine {
public:
    SimulationOutcome run(const ModuleDescriptor& descriptor, const SimulationPlan& plan) const;
};

}  // namespace pixelferrite

