#pragma once
#include "stock_data.h"
#include "gbm_engine.h"
#include <string>
#include <memory>

class ForecastServer {
public:
    ForecastServer(const std::string& data_dir,
                   const std::string& web_dir,
                   int port = 8080);
    void run(); // blocks until interrupted

private:
    std::string data_dir_;
    std::string web_dir_;
    int port_;

    std::unique_ptr<CSVDataSource> csv_source_;
    MonteCarloEngine engine_;

    std::string read_file(const std::string& path) const;
};
