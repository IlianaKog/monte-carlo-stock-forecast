# Monte Carlo Stock Forecast

A C++ application that forecasts stock prices using **Geometric Brownian Motion** and Monte Carlo simulation. Runs thousands of parallel price paths and exposes the results through a REST API consumed by a Vanilla JS frontend.

![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)
![CMake](https://img.shields.io/badge/build-CMake-blue.svg)
![License](https://img.shields.io/badge/license-MIT-green.svg)

---

## Features

- **GBM Monte Carlo engine** — simulates thousands of price paths using the closed-form GBM solution
- **OpenMP parallelism** — embarrassingly parallel simulation loop with thread-local PRNGs
- **Statistical analysis** — VaR 95%/99%, confidence bands (p5/p25/p50/p75/p95), probability of profit, expected return
- **REST API** — C++ HTTP server (cpp-httplib) serves both the API and the static frontend
- **Fan chart** — Chart.js visualization with percentile bands
- **Extensible data layer** — abstract `DataSource` interface; CSV implemented, Alpha Vantage stubbed

---

## Tech Stack

| Layer | Technology |
|---|---|
| Simulation engine | C++17, OpenMP |
| HTTP server | [cpp-httplib](https://github.com/yhirose/cpp-httplib) (header-only) |
| JSON | [nlohmann/json](https://github.com/nlohmann/json) (header-only) |
| Build | CMake 3.16+ |
| Frontend | Vanilla JS, [Chart.js](https://www.chartjs.org/) |
| Data | Historical CSV via [yfinance](https://github.com/ranaroussi/yfinance) |

---

## Mathematical Model

```
S(t + Δt) = S(t) · exp((μ - σ²/2)·Δt + σ·√Δt·Z)

  Z     ~ N(0,1)   Wiener process increment
  μ      mean daily log return  (estimated from historical data)
  σ      std dev of daily log returns
  Δt   = 1/252     one trading day
```

Parameters μ and σ are estimated from the historical closing prices supplied at runtime.

---

## Project Structure

```
monte-carlo-stock-forecast/
├── CMakeLists.txt
├── include/
│   ├── stock_data.h        # StockData struct, CSVDataSource, DataSource interface
│   ├── gbm_engine.h        # MonteCarloEngine — runs parallel GBM simulation
│   ├── statistics.h        # SimulationResult, Statistics — VaR, percentiles
│   ├── json_exporter.h     # Serialises SimulationResult to JSON
│   └── server.h            # ForecastServer wrapping cpp-httplib
├── src/
│   ├── main.cpp
│   ├── stock_data.cpp
│   ├── gbm_engine.cpp
│   ├── statistics.cpp
│   ├── json_exporter.cpp
│   └── server.cpp
├── web/                    # Frontend (served as static files by the C++ server)
│   ├── index.html
│   ├── app.js
│   └── style.css
├── third_party/
│   ├── httplib.h           # cpp-httplib v0.18
│   └── json.hpp            # nlohmann/json v3.11
├── tests/
│   ├── test_gbm.cpp
│   └── test_statistics.cpp
├── data/                   # Historical CSV files (Date, Open, High, Low, Close, Volume)
└── download_data.py        # Downloads historical data via yfinance
```

---

## Prerequisites

- **CMake** 3.16+
- **g++** with C++17 support (GCC 9+ recommended)
- **OpenMP** (`libomp-dev` on Ubuntu)
- **Python 3.8+** with `yfinance` (only for downloading data)

On Ubuntu / WSL2:
```bash
sudo apt-get install cmake g++ libomp-dev
```

---

## Build

```bash
# 1. Clone and enter the project
git clone https://github.com/your-username/monte-carlo-stock-forecast.git
cd monte-carlo-stock-forecast

# 2. Download historical data (requires yfinance)
pip install yfinance
python download_data.py        # saves AAPL.csv, MSFT.csv, GOOGL.csv to data/

# 3. Build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j$(nproc)

# 4. Run tests
./build/tests/test_runner

# 5. Start the server
./build/forecast               # listening on http://localhost:8080
```

Or use the convenience script:

```bash
chmod +x build.sh && ./build.sh
```

---

## API Reference

| Method | Endpoint | Description |
|---|---|---|
| `GET` | `/` | Web UI |
| `GET` | `/api/health` | Health check |
| `GET` | `/api/list-csv` | List available symbols |
| `GET` | `/api/prices?symbol=AAPL` | Return historical closing prices |
| `POST` | `/api/simulate` | Run Monte Carlo simulation |

### POST /api/simulate

**Request**
```json
{
  "symbol": "AAPL",
  "days": 252,
  "simulations": 10000,
  "historical_prices": [150.0, 151.2, ...]
}
```

**Response**
```json
{
  "symbol": "AAPL",
  "current_price": 175.5,
  "parameters": { "mu": 0.0012, "sigma": 0.0186 },
  "statistics": {
    "mean_final_price": 195.3,
    "var_95": 125.2,
    "var_99": 108.5,
    "probability_of_profit": 0.68,
    "expected_return_pct": 11.28,
    "percentiles": { "p5": 118.5, "p25": 155.2, "p50": 188.9, "p75": 225.4, "p95": 285.1 }
  },
  "paths": {
    "mean": [...],
    "p5": [...], "p25": [...], "p75": [...], "p95": [...]
  }
}
```

---

## Adding Your Own Data

The CSV parser expects a `Close` column:

```
Date,Open,High,Low,Close,Volume
2024-01-02,185.22,186.74,183.92,185.52,55228900
```

Download from Yahoo Finance manually, or add a ticker to `download_data.py` and re-run it. Any `.csv` file placed in `data/` will appear automatically in the UI dropdown.

---

## Performance Notes

**Thread-local PRNGs, not a shared mutex.**
Each OpenMP thread owns its own `std::mt19937_64`, seeded with `base_seed + thread_id`. The alternative — a single shared generator behind a `std::mutex` — would serialize every random number draw, causing lock contention that eliminates any parallelism gain and adds OS context-switch overhead on top. Thread-local generators cost nothing at synchronization and scale linearly with core count.

**Row-major memory layout for cache efficiency.**
Simulation paths are stored as `paths[sim_index][day_index]`. The inner loop always advances along the day axis (`d = 0 … N`), so each iteration reads the next element in the same cache line already loaded from the previous step. On a typical 64-byte cache line (8 doubles), the CPU prefetches 8 days at once — subsequent accesses are L1 hits. A transposed layout (`paths[day][sim]`) would cause a cache miss on every step of every path.

---

## License

MIT
