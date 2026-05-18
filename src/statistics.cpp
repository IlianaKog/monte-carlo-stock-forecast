#include "statistics.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>

double Statistics::percentile(std::vector<double> v, double pct) {
    if (v.empty()) throw std::runtime_error("Empty vector in percentile()");
    std::sort(v.begin(), v.end());
    double idx = pct * (static_cast<double>(v.size()) - 1.0);
    size_t lo  = static_cast<size_t>(idx);
    size_t hi  = lo + 1;
    if (hi >= v.size()) return v.back();
    double frac = idx - static_cast<double>(lo);
    return v[lo] * (1.0 - frac) + v[hi] * frac; // linear interpolation
}

std::vector<double> Statistics::day_percentile_path(const SimulationPaths& sim, double pct) {
    int days = sim.num_days + 1;
    std::vector<double> path(days);
    std::vector<double> col(sim.num_simulations);
    for (int d = 0; d < days; ++d) {
        for (int i = 0; i < sim.num_simulations; ++i)
            col[i] = sim.paths[i][d];
        path[d] = percentile(col, pct);
    }
    return path;
}

std::vector<double> Statistics::day_mean_path(const SimulationPaths& sim) {
    int days = sim.num_days + 1;
    std::vector<double> path(days, 0.0);
    for (int i = 0; i < sim.num_simulations; ++i)
        for (int d = 0; d < days; ++d)
            path[d] += sim.paths[i][d];
    for (auto& v : path) v /= sim.num_simulations;
    return path;
}

SimulationResult Statistics::compute(const std::string& symbol,
                                     const GBMParams& params,
                                     const SimulationPaths& sim) {
    const int n = sim.num_simulations;

    // collect final prices
    std::vector<double> finals(n);
    for (int i = 0; i < n; ++i)
        finals[i] = sim.paths[i][sim.num_days];

    double mean = std::accumulate(finals.begin(), finals.end(), 0.0) / n;

    double variance = 0.0;
    for (double f : finals) variance += (f - mean) * (f - mean);
    variance /= (n - 1.0);

    double profit_count = 0.0;
    for (double f : finals)
        if (f > params.S0) profit_count += 1.0;

    SimulationResult r;
    r.symbol              = symbol;
    r.num_simulations     = n;
    r.num_days            = sim.num_days;
    r.current_price       = params.S0;
    r.mu                  = params.mu;
    r.sigma               = params.sigma;
    r.mean_final_price    = mean;
    r.std_dev             = std::sqrt(variance);
    r.var_95              = percentile(finals, 0.05); // worst 5%
    r.var_99              = percentile(finals, 0.01); // worst 1%
    r.probability_of_profit  = profit_count / n;
    r.expected_return_pct    = (mean / params.S0 - 1.0) * 100.0;
    r.final_percentiles   = {
        percentile(finals, 0.05),
        percentile(finals, 0.25),
        percentile(finals, 0.50),
        percentile(finals, 0.75),
        percentile(finals, 0.95)
    };
    r.path_mean = day_mean_path(sim);
    r.path_p5   = day_percentile_path(sim, 0.05);
    r.path_p25  = day_percentile_path(sim, 0.25);
    r.path_p75  = day_percentile_path(sim, 0.75);
    r.path_p95  = day_percentile_path(sim, 0.95);

    return r;
}
