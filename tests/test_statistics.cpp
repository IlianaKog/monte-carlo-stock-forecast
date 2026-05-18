#include "statistics.h"
#include "gbm_engine.h"
#include <cassert>
#include <cmath>
#include <iostream>

// Single definition of the shared failure counter (used by both test files)
int failures = 0;
extern int run_gbm_tests();

#define EXPECT_TRUE(cond) \
    do { if (!(cond)) { std::cerr << "FAIL [" << __FILE__ << ":" << __LINE__ << "] " #cond "\n"; ++failures; } } while(0)

#define EXPECT_NEAR(a, b, tol) \
    do { if (std::abs((a)-(b)) > (tol)) { \
        std::cerr << "FAIL [" << __FILE__ << ":" << __LINE__ << "] |" << (a) << " - " << (b) << "| > " << (tol) << "\n"; \
        ++failures; } } while(0)

// ── Tests ─────────────────────────────────────────────────────────────────────

void test_statistics_dimensions() {
    MonteCarloEngine engine(1);
    GBMParams p{0.0005, 0.015, 100.0};
    auto sim = engine.run(p, 30, 500);
    auto result = Statistics::compute("TEST", p, sim);

    EXPECT_TRUE(result.num_simulations == 500);
    EXPECT_TRUE(result.num_days == 30);
    EXPECT_TRUE((int)result.path_mean.size() == 31);
    EXPECT_TRUE((int)result.path_p5.size()   == 31);
    EXPECT_TRUE((int)result.path_p95.size()  == 31);
}

void test_statistics_starting_price() {
    MonteCarloEngine engine(2);
    GBMParams p{0.0, 0.01, 150.0};
    auto sim = engine.run(p, 10, 100);
    auto result = Statistics::compute("X", p, sim);
    // Day 0 of every path is S0, so all day-0 stats must equal S0
    EXPECT_NEAR(result.path_mean[0], 150.0, 1e-9);
    EXPECT_NEAR(result.path_p5[0],   150.0, 1e-9);
    EXPECT_NEAR(result.path_p95[0],  150.0, 1e-9);
    EXPECT_NEAR(result.current_price, 150.0, 1e-9);
}

void test_statistics_var_ordering() {
    MonteCarloEngine engine(3);
    GBMParams p{0.001, 0.02, 100.0};
    auto sim = engine.run(p, 252, 5000);
    auto result = Statistics::compute("Y", p, sim);
    // VaR99 <= VaR95 <= p25 <= p50 <= p75
    EXPECT_TRUE(result.var_99 <= result.var_95);
    EXPECT_TRUE(result.var_95 <= result.final_percentiles.p50);
    EXPECT_TRUE(result.final_percentiles.p25 <= result.final_percentiles.p50);
    EXPECT_TRUE(result.final_percentiles.p50 <= result.final_percentiles.p75);
    EXPECT_TRUE(result.final_percentiles.p75 <= result.final_percentiles.p95);
}

void test_probability_of_profit_bounds() {
    MonteCarloEngine engine(4);
    GBMParams p{0.001, 0.02, 100.0};
    auto sim = engine.run(p, 252, 2000);
    auto result = Statistics::compute("Z", p, sim);
    EXPECT_TRUE(result.probability_of_profit >= 0.0);
    EXPECT_TRUE(result.probability_of_profit <= 1.0);
}

void test_high_drift_mostly_profit() {
    MonteCarloEngine engine(5);
    // Very high drift → almost all paths should be profitable
    GBMParams p{0.05, 0.01, 100.0};
    auto sim = engine.run(p, 252, 2000);
    auto result = Statistics::compute("UP", p, sim);
    EXPECT_TRUE(result.probability_of_profit > 0.99);
    EXPECT_TRUE(result.expected_return_pct > 0.0);
}

int run_statistics_tests() {
    test_statistics_dimensions();
    test_statistics_starting_price();
    test_statistics_var_ordering();
    test_probability_of_profit_bounds();
    test_high_drift_mostly_profit();
    return failures;
}

int main() {
    int f = run_gbm_tests();
    f += run_statistics_tests();
    if (f == 0)
        std::cout << "All tests passed.\n";
    else
        std::cerr << f << " test(s) FAILED.\n";
    return f > 0 ? 1 : 0;
}
