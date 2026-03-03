#pragma once

#include <string>
#include <vector>

#include <pixelferrite/module_loader.hpp>
#include <pixelferrite/simulation_engine.hpp>

namespace pixelferrite {

struct ExplainRequest {
    ModuleDescriptor descriptor;
    SimulationPlan plan;
    SimulationOutcome outcome;
    std::vector<std::string> missing_required;
    bool scope_enforced = false;
};

class ExplainEngine {
public:
    std::string summary(const ExplainRequest& request) const;
    std::vector<std::string> details(const ExplainRequest& request) const;
};

}  // namespace pixelferrite

