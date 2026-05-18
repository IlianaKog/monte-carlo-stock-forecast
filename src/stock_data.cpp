#include "stock_data.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cmath>
#include <filesystem>

namespace fs = std::filesystem;

// ── free function ─────────────────────────────────────────────────────────────

std::vector<double> compute_log_returns(const std::vector<double>& prices) {
    if (prices.size() < 2)
        throw std::runtime_error("Need at least 2 prices to compute log returns");
    std::vector<double> returns;
    returns.reserve(prices.size() - 1);
    for (size_t i = 1; i < prices.size(); ++i)
        returns.push_back(std::log(prices[i] / prices[i - 1]));
    return returns;
}

// ── CSVDataSource ─────────────────────────────────────────────────────────────

CSVDataSource::CSVDataSource(const std::string& data_dir)
    : data_dir_(data_dir) {}

StockData CSVDataSource::fetch(const std::string& symbol) {
    std::string path = data_dir_ + "/" + symbol + ".csv";
    std::ifstream file(path);
    if (!file.is_open())
        throw std::runtime_error("Cannot open CSV: " + path);

    StockData data;
    data.symbol = symbol;

    std::string line;
    std::getline(file, line); // skip header: Date,Open,High,Low,Close,Volume

    int close_col = -1;
    {
        std::istringstream hdr(line);
        std::string token;
        int col = 0;
        while (std::getline(hdr, token, ',')) {
            if (token == "Close" || token == "close") { close_col = col; break; }
            ++col;
        }
    }
    if (close_col < 0)
        throw std::runtime_error("CSV missing 'Close' column: " + path);

    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::istringstream row(line);
        std::string token;
        int col = 0;
        while (std::getline(row, token, ',')) {
            if (col == close_col && !token.empty() && token != "null") {
                try { data.prices.push_back(std::stod(token)); }
                catch (...) {}
            }
            ++col;
        }
    }

    if (data.prices.size() < 2)
        throw std::runtime_error("Not enough price rows in: " + path);

    data.log_returns = compute_log_returns(data.prices);
    return data;
}

std::vector<std::string> CSVDataSource::list_available() const {
    std::vector<std::string> names;
    for (const auto& entry : fs::directory_iterator(data_dir_)) {
        if (entry.path().extension() == ".csv") {
            names.push_back(entry.path().stem().string());
        }
    }
    return names;
}

// ── AlphaVantageSource stub ───────────────────────────────────────────────────

AlphaVantageSource::AlphaVantageSource(const std::string& api_key)
    : api_key_(api_key) {}

StockData AlphaVantageSource::fetch(const std::string& /*symbol*/) {
    throw std::runtime_error("AlphaVantage integration not yet implemented");
}
