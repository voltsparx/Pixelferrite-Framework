#include <pixelferrite/simulation_engine.hpp>

#include <algorithm>
#include <cstdint>
#include <functional>
#include <numeric>
#include <string_view>

namespace pixelferrite {

namespace {

std::uint64_t stable_hash(std::string_view text) {
    return static_cast<std::uint64_t>(std::hash<std::string_view>{}(text));
}

int clamp_score(int value) {
    return std::clamp(value, 0, 100);
}

int bounded(std::uint64_t seed, int min_value, int max_value) {
    if (max_value <= min_value) {
        return min_value;
    }
    const std::uint64_t span = static_cast<std::uint64_t>(max_value - min_value + 1);
    return min_value + static_cast<int>(seed % span);
}

std::string pick_profile(const std::string& category, const std::string& action) {
    if (action == "check") {
        return "readiness_audit";
    }
    if (category == "evasion") {
        return "stealth_balanced";
    }
    if (category == "payload" || category == "exploit") {
        return "precision_delivery";
    }
    if (category == "auxiliary") {
        return "telemetry_collection";
    }
    if (category == "analysis" || category == "detection") {
        return "deep_observability";
    }
    return "balanced_simulation";
}

int category_bonus(const std::string& category, int for_success) {
    if (for_success != 0) {
        if (category == "payload") {
            return 6;
        }
        if (category == "auxiliary") {
            return 8;
        }
        if (category == "evasion") {
            return 4;
        }
        if (category == "detection" || category == "analysis") {
            return 10;
        }
    } else {
        if (category == "exploit") {
            return 12;
        }
        if (category == "payload") {
            return 8;
        }
        if (category == "auxiliary") {
            return 4;
        }
    }
    return 0;
}

std::string quality_band_for_score(int score) {
    if (score >= 85) {
        return "elite";
    }
    if (score >= 70) {
        return "strong";
    }
    if (score >= 55) {
        return "stable";
    }
    return "degraded";
}

}  // namespace

SimulationOutcome SimulationEngine::run(const ModuleDescriptor& descriptor, const SimulationPlan& plan) const {
    SimulationOutcome outcome;
    outcome.execution_profile = pick_profile(descriptor.category, plan.action);

    const int normalized_intensity = std::clamp(plan.intensity, 30, 100);
    int stability_sum = 0;
    int confidence_sum = 0;

    for (const std::string& target : plan.targets) {
        const std::string seed_input =
            descriptor.id + "|" + plan.action + "|" + plan.transport + "|" + target + "|" + plan.platform;
        const std::uint64_t seed = stable_hash(seed_input);

        SimulationTargetResult result;
        result.target = target;
        result.ip_version = (target.find(':') != std::string::npos) ? "ipv6" :
                            (target.find('.') != std::string::npos) ? "ipv4" : "label";

        if (result.ip_version == "ipv4") {
            ++outcome.ipv4_targets;
        } else if (result.ip_version == "ipv6") {
            ++outcome.ipv6_targets;
        }

        const int latency_base = bounded(seed >> 2, 12, 95);
        const int jitter_base = bounded(seed >> 11, 1, 30);
        result.latency_ms = latency_base + ((result.ip_version == "ipv6") ? 4 : 0);
        result.jitter_ms = jitter_base + ((result.ip_version == "ipv6") ? 2 : 0);

        const int success_base = 58 + bounded(seed >> 19, 0, 34);
        result.success_probability = clamp_score(
            success_base +
            category_bonus(descriptor.category, 1) +
            (normalized_intensity - 70) / 4 -
            result.jitter_ms / 6
        );

        const int stealth_base = 50 + bounded(seed >> 27, 0, 35);
        result.stealth_score = clamp_score(
            stealth_base +
            ((descriptor.category == "evasion") ? 12 : 0) -
            ((descriptor.category == "exploit") ? 8 : 0)
        );

        const int risk_base = 35 + bounded(seed >> 33, 0, 35);
        result.detection_risk = clamp_score(
            risk_base +
            category_bonus(descriptor.category, 0) +
            (normalized_intensity - 70) / 5 -
            result.stealth_score / 10
        );

        result.quality_band = quality_band_for_score(result.success_probability);

        stability_sum += result.success_probability;
        confidence_sum += (100 - result.detection_risk + result.stealth_score) / 2;
        outcome.target_results.push_back(std::move(result));
    }

    if (!outcome.target_results.empty()) {
        const int target_count = static_cast<int>(outcome.target_results.size());
        outcome.overall_stability = stability_sum / target_count;
        outcome.confidence_score = clamp_score(confidence_sum / target_count);
    }

    outcome.dual_stack_ready = outcome.ipv4_targets > 0 && outcome.ipv6_targets > 0;

    outcome.timeline.push_back("stage=precheck status=ok action=" + plan.action);
    outcome.timeline.push_back(
        "stage=targeting status=ok targets=" + std::to_string(plan.targets.size()) +
        " ipv4=" + std::to_string(outcome.ipv4_targets) +
        " ipv6=" + std::to_string(outcome.ipv6_targets)
    );
    outcome.timeline.push_back(
        "stage=delivery status=simulated transport=" + plan.transport +
        " profile=" + outcome.execution_profile
    );
    outcome.timeline.push_back(
        "stage=post status=ok confidence=" + std::to_string(outcome.confidence_score) +
        " stability=" + std::to_string(outcome.overall_stability)
    );

    return outcome;
}

}  // namespace pixelferrite

