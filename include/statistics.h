#pragma once
#include "gbm_engine.h"
#include <vector>
#include <string>

struct Percentiles {
    double p5, p25, p50, p75, p95;
};

struct SimulationResult {
    std::string symbol;
    int num_simulations;
    int num_days;
    double current_price;

    // GBM params
    double mu;
    double sigma;

    // aggregate stats
    double mean_final_price;
    double std_dev;
    double var_95;    // Value at Risk — 5th percentile of final prices
    double var_99;    // 1st percentile
    double probability_of_profit;
    double expected_return_pct;
    Percentiles final_percentiles;

    // representative paths (day-by-day), length = num_days + 1
    std::vector<double> path_mean;
    std::vector<double> path_p5;
    std::vector<double> path_p25;
    std::vector<double> path_p75;
    std::vector<double> path_p95;
};

class Statistics {
public:
    static SimulationResult compute(const std::string& symbol,
                                    const GBMParams& params,
                                    const SimulationPaths& sim);

private:
    static double percentile(std::vector<double> v, double pct); // by value — sorts copy, used for day paths
    static double percentile_sorted(const std::vector<double>& sorted_v, double pct); // O(1) — requires pre-sorted input
    static std::vector<double> day_percentile_path(const SimulationPaths& sim, double pct);
    static std::vector<double> day_mean_path(const SimulationPaths& sim);
};
