#include "gbm_engine.h"
#include <cmath>
#include <numeric>
#include <random>
#include <stdexcept>

#ifdef _OPENMP
#include <omp.h>
#endif

MonteCarloEngine::MonteCarloEngine(uint64_t seed) : seed_(seed) {}

GBMParams MonteCarloEngine::estimate_params(const std::vector<double>& log_returns,
                                            double last_price) const {
    if (log_returns.empty())
        throw std::runtime_error("Cannot estimate GBM params from empty returns");

    const double n = static_cast<double>(log_returns.size());
    const double mu = std::accumulate(log_returns.begin(), log_returns.end(), 0.0) / n;

    double variance = 0.0;
    for (double r : log_returns)
        variance += (r - mu) * (r - mu);
    variance /= (n - 1.0);

    return {mu, std::sqrt(variance), last_price};
}

SimulationPaths MonteCarloEngine::run(const GBMParams& params,
                                      int num_days,
                                      int num_simulations) const {
    if (num_days <= 0 || num_simulations <= 0)
        throw std::runtime_error("num_days and num_simulations must be positive");

    SimulationPaths result;
    result.num_simulations = num_simulations;
    result.num_days = num_days;
    result.paths.assign(num_simulations, std::vector<double>(num_days + 1));

    const double dt     = 1.0 / 252.0;
    const double drift  = (params.mu - 0.5 * params.sigma * params.sigma) * dt;
    const double vol_dt = params.sigma * std::sqrt(dt);
    const double S0     = params.S0;

    // RNG initialized once per thread, not per path --> to avoid overhead and
    // seed-reuse bias that would arise if we seeded inside the inner loop.
#pragma omp parallel
    {
#ifdef _OPENMP
        const uint64_t thread_seed = seed_ + static_cast<uint64_t>(omp_get_thread_num());
#else
        const uint64_t thread_seed = seed_;
#endif
        std::mt19937_64 rng(thread_seed);
        std::normal_distribution<double> dist(0.0, 1.0);

#pragma omp for schedule(dynamic, 64)
        for (int i = 0; i < num_simulations; ++i) {
            result.paths[i][0] = S0;
            for (int d = 1; d <= num_days; ++d) {
                result.paths[i][d] = result.paths[i][d - 1]
                                   * std::exp(drift + vol_dt * dist(rng));
            }
        }
    }

    return result;
}
