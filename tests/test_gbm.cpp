#include "gbm_engine.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <numeric>

// Minimal test harness — no external framework
// Defined in test_statistics.cpp (the translation unit that contains main)
extern int failures;

#define EXPECT_TRUE(cond) \
    do { if (!(cond)) { std::cerr << "FAIL [" << __FILE__ << ":" << __LINE__ << "] " #cond "\n"; ++failures; } } while(0)

#define EXPECT_NEAR(a, b, tol) \
    do { if (std::abs((a)-(b)) > (tol)) { \
        std::cerr << "FAIL [" << __FILE__ << ":" << __LINE__ << "] |" << (a) << " - " << (b) << "| > " << (tol) << "\n"; \
        ++failures; } } while(0)

// ── Tests ─────────────────────────────────────────────────────────────────────

void test_estimate_params_known_data() {
    // Constant log return of 0.01 every day → mu = 0.01, sigma = 0
    MonteCarloEngine engine;
    std::vector<double> returns(100, 0.01);
    auto params = engine.estimate_params(returns, 100.0);
    EXPECT_NEAR(params.mu, 0.01, 1e-10);
    EXPECT_NEAR(params.sigma, 0.0, 1e-6);
    EXPECT_NEAR(params.S0, 100.0, 1e-10);
}

void test_estimate_params_mixed() {
    // Alternating +0.02 / -0.02 → mu = 0, sigma > 0
    MonteCarloEngine engine;
    std::vector<double> returns;
    for (int i = 0; i < 100; ++i) returns.push_back(i % 2 == 0 ? 0.02 : -0.02);
    auto params = engine.estimate_params(returns, 50.0);
    EXPECT_NEAR(params.mu, 0.0, 1e-10);
    EXPECT_TRUE(params.sigma > 0.0);
}

void test_run_dimensions() {
    MonteCarloEngine engine(1);
    GBMParams p{0.0005, 0.015, 150.0};
    auto sim = engine.run(p, 30, 200);
    EXPECT_TRUE(sim.num_simulations == 200);
    EXPECT_TRUE(sim.num_days == 30);
    EXPECT_TRUE((int)sim.paths.size() == 200);
    EXPECT_TRUE((int)sim.paths[0].size() == 31); // days+1 (includes S0)
}

void test_run_starts_at_S0() {
    MonteCarloEngine engine(42);
    GBMParams p{0.0, 0.01, 200.0};
    auto sim = engine.run(p, 10, 50);
    for (int i = 0; i < 50; ++i)
        EXPECT_NEAR(sim.paths[i][0], 200.0, 1e-10);
}

void test_run_prices_positive() {
    // With any reasonable params, all simulated prices must be > 0 (GBM property)
    MonteCarloEngine engine(7);
    GBMParams p{-0.01, 0.05, 100.0};
    auto sim = engine.run(p, 252, 1000);
    for (int i = 0; i < 1000; ++i)
        for (int d = 0; d <= 252; ++d)
            EXPECT_TRUE(sim.paths[i][d] > 0.0);
}

void test_run_zero_drift_mean() {
    // With drift=0 (mu = sigma²/2) the expected price equals S0
    MonteCarloEngine engine(0);
    const double sigma = 0.02;
    GBMParams p{0.5 * sigma * sigma, sigma, 100.0};
    auto sim = engine.run(p, 252, 10000);
    double sum = 0.0;
    for (int i = 0; i < 10000; ++i) sum += sim.paths[i][252];
    double mean = sum / 10000.0;
    // With 10k sims and zero drift, mean should be within ±2% of S0
    EXPECT_NEAR(mean, 100.0, 2.0);
}

// Entry point shared with test_statistics.cpp — defined there
extern int run_statistics_tests();

int run_gbm_tests() {
    test_estimate_params_known_data();
    test_estimate_params_mixed();
    test_run_dimensions();
    test_run_starts_at_S0();
    test_run_prices_positive();
    test_run_zero_drift_mean();
    return failures;
}
