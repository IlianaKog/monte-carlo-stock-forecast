#pragma once
#include <string>
#include <vector>

struct StockData {
    std::string symbol;
    std::vector<double> prices;   // closing prices, chronological
    std::vector<double> log_returns;
};

class DataSource {
public:
    virtual StockData fetch(const std::string& symbol) = 0;
    virtual ~DataSource() = default;
};

class CSVDataSource : public DataSource {
public:
    explicit CSVDataSource(const std::string& data_dir);
    StockData fetch(const std::string& symbol) override;
    std::vector<std::string> list_available() const;

private:
    std::string data_dir_;
};

// future stub — not yet implemented
class AlphaVantageSource : public DataSource {
public:
    explicit AlphaVantageSource(const std::string& api_key);
    StockData fetch(const std::string& symbol) override;

private:
    std::string api_key_;
};

std::vector<double> compute_log_returns(const std::vector<double>& prices);
