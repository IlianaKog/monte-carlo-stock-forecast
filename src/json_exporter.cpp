#include "json_exporter.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// convert std::vector<double> to json array
static json vec_to_json(const std::vector<double>& v) {
    return json(v);
}

std::string JsonExporter::to_json(const SimulationResult& r) {
    json j;
    j["symbol"]          = r.symbol;
    j["simulations"]     = r.num_simulations;
    j["days"]            = r.num_days;
    j["current_price"]   = r.current_price;

    j["parameters"] = {
        {"mu",    r.mu},
        {"sigma", r.sigma}
    };

    j["statistics"] = {
        {"mean_final_price",      r.mean_final_price},
        {"std_dev",               r.std_dev},
        {"var_95",                r.var_95},
        {"var_99",                r.var_99},
        {"probability_of_profit", r.probability_of_profit},
        {"expected_return_pct",   r.expected_return_pct},
        {"percentiles", {
            {"p5",  r.final_percentiles.p5},
            {"p25", r.final_percentiles.p25},
            {"p50", r.final_percentiles.p50},
            {"p75", r.final_percentiles.p75},
            {"p95", r.final_percentiles.p95}
        }}
    };

    j["paths"] = {
        {"mean", vec_to_json(r.path_mean)},
        {"p5",   vec_to_json(r.path_p5)},
        {"p25",  vec_to_json(r.path_p25)},
        {"p75",  vec_to_json(r.path_p75)},
        {"p95",  vec_to_json(r.path_p95)}
    };

    return j.dump();
}
