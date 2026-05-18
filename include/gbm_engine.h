#pragma once
#include <vector>
#include <cstdint>

struct GBMParams {
    double mu;     // mean daily log return
    double sigma;  // std dev of daily log returns
    double S0;     // starting price
};

struct SimulationPaths {
    int num_simulations;
    int num_days;
    // each row is one full path: paths[sim_index][day_index]
    std::vector<std::vector<double>> paths;
};

class MonteCarloEngine {
public:
    explicit MonteCarloEngine(uint64_t seed = 42);

    GBMParams estimate_params(const std::vector<double>& log_returns,
                              double last_price) const;

    SimulationPaths run(const GBMParams& params,
                        int num_days,
                        int num_simulations) const;

private:
    uint64_t seed_;
};
