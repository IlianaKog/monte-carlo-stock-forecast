#pragma once
#include "statistics.h"
#include <string>

class JsonExporter {
public:
    // Returns the simulation result as a JSON string
    static std::string to_json(const SimulationResult& result);
};
