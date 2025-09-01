// Binance_pro_EmaHedging.cpp
// Converted from Python to C++
// Requires libcurl and nlohmann/json for HTTP and JSON parsing
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace std;

// Helper for libcurl response
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Fetch Binance klines
json fetch_binance_klines(const string& symbol, const string& interval, int limit) {
    string url = "https://api.binance.com/api/v3/klines?symbol=" + symbol + "&interval=" + interval + "&limit=" + to_string(limit);
    CURL* curl = curl_easy_init();
    string readBuffer;
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
    return json::parse(readBuffer);
}

vector<double> calculate_ema(const vector<double>& data, int period) {
    vector<double> ema(data.size(), 0.0);
    double alpha = 2.0 / (period + 1);
    ema[0] = data[0];
    for(size_t i = 1; i < data.size(); ++i) {
        ema[i] = alpha * data[i] + (1 - alpha) * ema[i-1];
    }
    return ema;
}

struct Trade {
    string entry_time;
    string exit_time;
    string direction;
    double entry_price;
    double exit_price;
    double pnl;
    string reason;
    int cycle;
    double duration_mins;
    string symbol;
};

void backtest_ema_strategy_binance(const string& symbol, int days = 3, double target_cycle_pnl = 1.7, double lot_size = 0.1, double spread = 2.0, int ema_period = 200, const string& interval = "5m") {
    cout << "Fetching data for " << symbol << " from Binance..." << endl;
    json klines = fetch_binance_klines(symbol, interval, 1000);
    if(klines.size() < ema_period + 2) {
        cout << "Not enough data for " << symbol << ". Skipping." << endl;
        return;
    }
    vector<double> closes, opens;
    vector<string> times;
    for(auto& k : klines) {
        opens.push_back(stod(k[1].get<string>()));
        closes.push_back(stod(k[4].get<string>()));
        times.push_back(to_string(k[0].get<long long>()));
    }
    vector<double> ema = calculate_ema(closes, ema_period);
    int position = 0;
    double entry_price = 0.0;
    string entry_time;
    double cycle_pnl = 0.0;
    int cycle_number = 1;
    vector<Trade> trades;
    int start_index = ema_period + 1;
    bool signal_detected = false;
    string signal_type;
    size_t i = start_index;
    while(i < closes.size()) {
        double current_open = opens[i];
        double current_close = closes[i];
        double current_ema = ema[i];
        double prev_close = closes[i-1];
        string current_time = times[i];
        if(position == 0) {
            bool long_condition = ((current_open < current_ema) || (prev_close < current_ema)) && (current_close > current_ema);
            bool short_condition = ((current_open > current_ema) || (prev_close > current_ema)) && (current_close < current_ema);
            if(long_condition) {
                signal_detected = true;
                signal_type = "LONG";
            } else if(short_condition) {
                signal_detected = true;
                signal_type = "SHORT";
            }
        } else {
            bool long_condition = ((current_open < current_ema) || (prev_close < current_ema)) && (current_close > current_ema);
            bool short_condition = ((current_open > current_ema) || (prev_close > current_ema)) && (current_close < current_ema);
            if((position == -1 && long_condition) || (position == 1 && short_condition)) {
                double exit_price = position == 1 ? current_close - spread : current_close + spread;
                double pnl = (exit_price - entry_price) * (position == 1 ? 1 : -1) * lot_size - 2 * spread * lot_size;
                double duration_mins = 0.0;
                trades.push_back({entry_time, current_time, position == 1 ? "LONG" : "SHORT", entry_price, exit_price, pnl, "opposite_signal", cycle_number, duration_mins, symbol});
                cycle_pnl += pnl;
                position = 0;
                entry_price = 0.0;
                entry_time = "";
                if(cycle_pnl >= target_cycle_pnl) {
                    cycle_pnl = 0.0;
                    cycle_number++;
                }
                if(long_condition) {
                    signal_detected = true;
                    signal_type = "LONG";
                } else if(short_condition) {
                    signal_detected = true;
                    signal_type = "SHORT";
                }
            }
            double exit_price_check = position == 1 ? current_close - spread : current_close + spread;
            double unrealized_pnl = (exit_price_check - entry_price) * (position == 1 ? 1 : -1) * lot_size;
            if(cycle_pnl + unrealized_pnl >= target_cycle_pnl) {
                double pnl = unrealized_pnl - 2 * spread * lot_size;
                trades.push_back({entry_time, current_time, position == 1 ? "LONG" : "SHORT", entry_price, exit_price_check, pnl, "cycle_target_reached", cycle_number, 0.0, symbol});
                cycle_pnl = 0.0;
                position = 0;
                entry_price = 0.0;
                entry_time = "";
                cycle_number++;
            }
        }
        if(signal_detected && position == 0) {
            if(i + 1 < closes.size()) {
                size_t next_i = i + 1;
                double next_open = opens[next_i];
                string next_time = times[next_i];
                if(signal_type == "LONG") {
                    position = 1;
                    entry_price = next_open + spread;
                    entry_time = next_time;
                } else if(signal_type == "SHORT") {
                    position = -1;
                    entry_price = next_open - spread;
                    entry_time = next_time;
                }
                signal_detected = false;
                signal_type = "";
                i = next_i;
                continue;
            } else {
                signal_detected = false;
                signal_type = "";
            }
        }
        i++;
    }
    if(position != 0) {
        double final_price = closes.back();
        double final_exit_price = position == 1 ? final_price - spread : final_price + spread;
        double pnl = (final_exit_price - entry_price) * (position == 1 ? 1 : -1) * lot_size - 2 * spread * lot_size;
        trades.push_back({entry_time, times.back(), position == 1 ? "LONG" : "SHORT", entry_price, final_exit_price, pnl, "end_of_backtest", cycle_number, 0.0, symbol});
    }
    // Print summary
    cout << "\nTrades for " << symbol << ":\n";
    for(const auto& t : trades) {
        cout << "Cycle: " << t.cycle << ", Dir: " << t.direction << ", Entry: " << t.entry_price << ", Exit: " << t.exit_price << ", PnL: " << t.pnl << ", Reason: " << t.reason << endl;
    }
}

int main() {
    vector<string> symbols = {"BTCUSDT", "ETHUSDT"}; // Example top symbols
    for(const auto& symbol : symbols) {
        backtest_ema_strategy_binance(symbol);
    }
    return 0;
}
