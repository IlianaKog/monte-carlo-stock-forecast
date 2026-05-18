import yfinance as yf
import os

os.makedirs("data", exist_ok=True)

tickers = ["AAPL", "MSFT", "GOOGL"]

for ticker in tickers:
    print(f"Downloading {ticker}...")
    data = yf.download(ticker, period="5y", interval="1d", auto_adjust=True, progress=False)

    # flatten MultiIndex columns (yfinance 0.2+)
    if hasattr(data.columns, 'droplevel'):
        try:
            data.columns = data.columns.droplevel(1)
        except Exception:
            pass

    data = data[['Open', 'High', 'Low', 'Close', 'Volume']]
    data.index.name = 'Date'

    path = f"data/{ticker}.csv"
    data.to_csv(path)
    print(f"  Saved {len(data)} rows -> {path}")

print("\nDone.")
