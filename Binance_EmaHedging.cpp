// Binance_EmaHedging.cpp
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

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

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
    bool using_virtual_ema;
};

void backtest_directional_strategy_binance(const string& symbol, int days = 20, double target_cycle_pnl = 3.31, double commission_per_trade = 0.2, double profit_opt_threshold = -4.0, double profit_opt_target = 5.0, const string& interval = "1m") {
    cout << "Fetching data for " << symbol << " from Binance..." << endl;
    json klines = fetch_binance_klines(symbol, interval, 1000);
    if(klines.size() < 201) {
        cout << "Not enough data for " << symbol << ". Skipping." << endl;
        return;
    }
    vector<double> closes;
    vector<string> times;
    for(auto& k : klines) {
        closes.push_back(stod(k[4].get<string>()));
        times.push_back(to_string(k[0].get<long long>()));
    }
    vector<double> ema200 = calculate_ema(closes, 200);
    int position = 0;
    double lot_size = 0.7;
    double entry_price = 0.0;
    string entry_time;
    double cycle_pnl = 0.0;
    bool profit_opt_active = false;
    int cycle_number = 1;
    bool using_virtual_ema = false;
    double virtual_ema_offset = 0.0;
    int start_idx = 200;
    vector<Trade> trades;
    for(size_t i = start_idx; i < closes.size(); ++i) {
        string now = times[i];
        double price = closes[i];
        double ema = ema200[i];
        double ema_reference = using_virtual_ema ? ema + virtual_ema_offset : ema;
        if(cycle_pnl <= profit_opt_threshold && !using_virtual_ema) {
            profit_opt_active = true;
            if(profit_opt_active) {
                virtual_ema_offset = price - ema;
                using_virtual_ema = true;
                cout << "Activated virtual EMA at " << now << ", Price: " << price << ", EMA200: " << ema << ", Offset: " << virtual_ema_offset << endl;
            }
        }
        if(position == 0) {
            if(price > ema_reference) {
                position = 1;
                entry_price = price;
                entry_time = now;
                continue;
            } else if(price < ema_reference) {
                position = -1;
                entry_price = price;
                entry_time = now;
                continue;
            }
        }
        if(position != 0) {
            double unreal_pnl = (price - entry_price) * (position == 1 ? 1 : -1) * lot_size - commission_per_trade;
            if(profit_opt_active && unreal_pnl >= profit_opt_target) {
                trades.push_back({entry_time, now, position == 1 ? "LONG" : "SHORT", entry_price, price, unreal_pnl, "profit_opt", cycle_number, 0.0, using_virtual_ema});
                cycle_pnl += unreal_pnl;
                position = 0;
                entry_price = 0.0;
                entry_time = "";
                continue;
            }
            if(cycle_pnl + unreal_pnl >= target_cycle_pnl) {
                trades.push_back({entry_time, now, position == 1 ? "LONG" : "SHORT", entry_price, price, unreal_pnl, "cycle_target", cycle_number, 0.0, using_virtual_ema});
                cycle_pnl = 0.0;
                profit_opt_active = false;
                using_virtual_ema = false;
                virtual_ema_offset = 0.0;
                cycle_number++;
                position = 0;
                entry_price = 0.0;
                entry_time = "";
                continue;
            }
            if(position == 1 && price < ema_reference) {
                trades.push_back({entry_time, now, "LONG", entry_price, price, unreal_pnl, "ema_cross", cycle_number, 0.0, using_virtual_ema});
                cycle_pnl += unreal_pnl;
                position = 0;
                entry_price = 0.0;
                entry_time = "";
                continue;
            }
            if(position == -1 && price > ema_reference) {
                trades.push_back({entry_time, now, "SHORT", entry_price, price, unreal_pnl, "ema_cross", cycle_number, 0.0, using_virtual_ema});
                cycle_pnl += unreal_pnl;
                position = 0;
                entry_price = 0.0;
                entry_time = "";
                continue;
            }
        }
    }
    if(position != 0) {
        double final_price = closes.back();
        trades.push_back({entry_time, times.back(), position == 1 ? "LONG" : "SHORT", entry_price, final_price, (final_price - entry_price) * (position == 1 ? 1 : -1) * lot_size - commission_per_trade, "end_of_backtest", cycle_number, 0.0, using_virtual_ema});
    }
    // Print summary
    cout << "\nTrades for " << symbol << ":\n";
    for(const auto& t : trades) {
        cout << "Cycle: " << t.cycle << ", Dir: " << t.direction << ", Entry: " << t.entry_price << ", Exit: " << t.exit_price << ", PnL: " << t.pnl << ", Reason: " << t.reason << ", Virtual EMA: " << (t.using_virtual_ema ? "Yes" : "No") << endl;
    }
}

int main() {
    vector<string> symbols = {"BTCUSDT", "ETHUSDT"}; // Example top symbols
    for(const auto& symbol : symbols) {
        backtest_directional_strategy_binance(symbol);
    }
    return 0;
}
