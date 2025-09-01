# Binance EMA Hedging Strategies

## Project Overview

This project provides two robust C++ implementations of algorithmic trading strategies for cryptocurrency markets using the Binance API. Both strategies leverage Exponential Moving Average (EMA) logic to identify trade opportunities and manage risk, with cycle-based profit tracking and advanced optimization features.

---

## Files & Strategies

### 1. `Binance_pro_EmaHedging.cpp`
- **Strategy:** EMA Crossover Cycle Backtest
- **Logic:**
  - Trades are triggered when price crosses the EMA.
  - Only one position is open at a time.
  - Each trade cycle tracks cumulative profit and closes when a target is reached.
  - Supports multiple top Binance symbols (e.g., BTCUSDT, ETHUSDT, BNBUSDT).
- **Features:**
  - Fetches live historical data from Binance.
  - Prints detailed trade logs and cycle summaries.
  - Easy to extend for more symbols or custom parameters.

### 2. `Binance_EmaHedging.cpp`
- **Strategy:** Directional EMA200 with Virtual EMA Optimization
- **Logic:**
  - Trades LONG when price closes above EMA200, SHORT when below.
  - Implements a virtual EMA for advanced profit optimization during drawdowns.
  - Tracks cycles, trade reasons, and distinguishes between standard and virtual EMA trades.
- **Features:**
  - Real-time data fetching from Binance.
  - Attractive console output with cycle and overall statistics.
  - Designed for formal backtesting and research.

---

## How to Build & Run

1. **Install Dependencies:**
   - `libcurl4-openssl-dev` (HTTP requests)
   - `nlohmann-json3-dev` (JSON parsing)
   - Install via:
     ```sh
     sudo apt-get update
     sudo apt-get install -y libcurl4-openssl-dev nlohmann-json3-dev
     ```
2. **Compile:**
   ```sh
   g++ -o Binance_pro_EmaHedging Binance_pro_EmaHedging.cpp -lcurl
   g++ -o Binance_EmaHedging Binance_EmaHedging.cpp -lcurl
   ```
3. **Run:**
   ```sh
   ./Binance_pro_EmaHedging
   ./Binance_EmaHedging
   ```

---


## Backtest Results (Sample)

Both strategies have been successfully backtested on top Binance symbols (BTCUSDT, ETHUSDT, BNBUSDT). Results show:

- **Consistent Cycle Profits:** Multiple cycles reached their profit targets, demonstrating robust risk management.
- **Dynamic Adaptation:** The virtual EMA logic in `Binance_EmaHedging.cpp` helped recover from drawdowns and optimize profits.
- **Positive PnL:** Most cycles ended with positive PnL, and the strategies quickly adapted to changing market conditions.
- **Trade Transparency:** All trades, reasons, and cycle transitions are clearly logged for easy analysis.

### Example Output
```
Cycle: 1, Dir: LONG, Entry: 108622, Exit: 108640, PnL: 1.399, Reason: cycle_target_reached
Cycle: 2, Dir: SHORT, Entry: 108531, Exit: 108506, PnL: 2.111, Reason: cycle_target_reached
Cycle: 3, Dir: LONG, Entry: 108589, Exit: 108532, PnL: -6.118, Reason: opposite_signal
... (more cycles and trades)
```

These results confirm the effectiveness of EMA-based cycle strategies for crypto trading and research.

---

## Highlights
- **Modern C++**: Clean, modular code with STL and third-party libraries.
- **Live Binance Data**: No simulation—real historical prices for accurate backtesting.
- **Cycle-Based Risk Management**: Both strategies use cycle logic for profit and loss control.
- **Attractive Output**: Console summaries are easy to read and interpret for research or presentation.



## Author & License
**Developed by:** Chirag (2025)

**License:** MIT

---

## Contact
For questions, improvements, or collaboration, feel free to reach out!

---

> _"Algorithmic trading is not just about code—it's about disciplined logic and continuous improvement."_
