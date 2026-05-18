#include "server.h"
#include "statistics.h"
#include "json_exporter.h"
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>

using json = nlohmann::json;

ForecastServer::ForecastServer(const std::string& data_dir,
                               const std::string& web_dir,
                               int port)
    : data_dir_(data_dir), web_dir_(web_dir), port_(port),
      csv_source_(std::make_unique<CSVDataSource>(data_dir)),
      engine_(42) {}

std::string ForecastServer::read_file(const std::string& path) const {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("Cannot read file: " + path);
    std::ostringstream buf;
    buf << f.rdbuf();
    return buf.str();
}

void ForecastServer::run() {
    httplib::Server svr;

    // ── CORS ────────────────────────────────────────────────────────────────
    svr.set_default_headers({
        {"Access-Control-Allow-Origin",  "*"},
        {"Access-Control-Allow-Headers", "Content-Type"}
    });

    // ── Static files ─────────────────────────────────────────────────────────
    svr.Get("/", [this](const httplib::Request&, httplib::Response& res) {
        res.set_content(read_file(web_dir_ + "/index.html"), "text/html");
    });
    svr.Get("/app.js", [this](const httplib::Request&, httplib::Response& res) {
        res.set_content(read_file(web_dir_ + "/app.js"), "application/javascript");
    });
    svr.Get("/style.css", [this](const httplib::Request&, httplib::Response& res) {
        res.set_content(read_file(web_dir_ + "/style.css"), "text/css");
    });

    // ── Health check ─────────────────────────────────────────────────────────
    svr.Get("/api/health", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(R"({"status":"ok","version":"1.0"})", "application/json");
    });

    // ── Historical prices for a symbol ───────────────────────────────────────
    svr.Get("/api/prices", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            if (!req.has_param("symbol")) {
                res.status = 400;
                res.set_content(json{{"error","missing ?symbol="}}.dump(), "application/json");
                return;
            }
            const std::string symbol = req.get_param_value("symbol");
            const auto data = csv_source_->fetch(symbol);
            json j;
            j["symbol"] = data.symbol;
            j["prices"] = json(data.prices);
            res.set_content(j.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 404;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        }
    });

    // ── List available CSVs ───────────────────────────────────────────────────
    svr.Get("/api/list-csv", [this](const httplib::Request&, httplib::Response& res) {
        try {
            auto names = csv_source_->list_available();
            json j = json::array();
            for (const auto& n : names) j.push_back(n);
            res.set_content(j.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 500;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        }
    });

    // ── Simulate ─────────────────────────────────────────────────────────────
    svr.Post("/api/simulate", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            auto body = json::parse(req.body);

            const std::string symbol       = body.value("symbol", "UNKNOWN");
            const int days                 = body.value("days", 252);
            const int simulations          = body.value("simulations", 10000);
            const auto& hp                 = body["historical_prices"];

            if (!hp.is_array() || hp.size() < 2) {
                res.status = 400;
                res.set_content(
                    json{{"error","historical_prices must be an array with ≥2 elements"}}.dump(),
                    "application/json");
                return;
            }

            std::vector<double> prices;
            prices.reserve(hp.size());
            for (const auto& v : hp) prices.push_back(v.get<double>());

            const auto log_returns = compute_log_returns(prices);
            const auto params      = engine_.estimate_params(log_returns, prices.back());
            const auto sim         = engine_.run(params, days, simulations);
            const auto result      = Statistics::compute(symbol, params, sim);

            res.set_content(JsonExporter::to_json(result), "application/json");

        } catch (const json::exception& e) {
            res.status = 400;
            res.set_content(json{{"error", std::string("JSON parse error: ") + e.what()}}.dump(),
                            "application/json");
        } catch (const std::exception& e) {
            res.status = 500;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        }
    });

    // ── OPTIONS preflight (CORS) ──────────────────────────────────────────────
    svr.Options(".*", [](const httplib::Request&, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.status = 204;
    });

    std::cout << "Monte Carlo Forecast server running on http://localhost:" << port_ << "\n";
    svr.listen("0.0.0.0", port_);
}
