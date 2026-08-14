import pandas as pd
import numpy as np
import os
import matplotlib.pyplot as plt

def run_backtest():
    # 1. Load the Strategy "Brain" (Optimal Quotes)
    quotes_path = "results/quotes_vs_Vpi.csv"
    if not os.path.exists(quotes_path):
        print(f"Error: {quotes_path} not found.")
        return
        
    df_quotes = pd.read_csv(quotes_path)
    
    # Create interpolation functions for delta_bid and delta_ask for each option
    # Since Vpi is the only state variable exported, we interpolate over it.
    option_brains = {}
    options_idx = df_quotes['option_idx'].unique()
    for idx in options_idx:
        df_opt = df_quotes[df_quotes['option_idx'] == idx].sort_values(by='Vpi')
        vpi_vals = df_opt['Vpi'].values
        bid_vals = df_opt['delta_bid'].values
        ask_vals = df_opt['delta_ask'].values
        
        # Store data arrays for interpolation later
        option_brains[idx] = {'vpi': vpi_vals, 'bid': bid_vals, 'ask': ask_vals}
        
    # 2. Load Historical Market Data
    data_path = "data/historical_data.csv"
    if not os.path.exists(data_path):
        print(f"Error: {data_path} not found. Run generate_mock_market_data.py first.")
        return
        
    print("Loading historical data...")
    df_market = pd.read_csv(data_path)
    
    # 3. Initialize Portfolio State and Run Backtest
    Vpi = 0.0
    pnl = 0.0
    trades_executed = 0
    pnl_history = []
    vpi_history = []
    times = []
    
    print("Running historical backtest tick-by-tick...")
    for index, row in df_market.iterrows():
        opt_idx = row['option_idx']
        trade_dir = row['trade_dir']
        trade_price = row['trade_price']
        mid_price = row['mid_price']
        z_i = row['z_i']
        vega_i = row['vega_i']
        
        delta_bid = float(np.interp(Vpi, option_brains[opt_idx]['vpi'], option_brains[opt_idx]['bid']))
        delta_ask = float(np.interp(Vpi, option_brains[opt_idx]['vpi'], option_brains[opt_idx]['ask']))
        
        our_bid_price = mid_price - delta_bid
        our_ask_price = mid_price + delta_ask
        
        trade_happened = False
        if trade_dir == -1 and trade_price <= our_bid_price:
            Vpi += z_i * vega_i
            pnl += z_i * delta_bid
            trade_happened = True
        elif trade_dir == 1 and trade_price >= our_ask_price:
            Vpi -= z_i * vega_i
            pnl += z_i * delta_ask
            trade_happened = True
            
        if trade_happened:
            trades_executed += 1
            
        pnl_history.append(pnl)
        vpi_history.append(Vpi)
        times.append(row['time'])
        
    print(f"Backtest completed! Executed {trades_executed} out of {len(df_market)} market ticks.")
    print(f"Final Captured Spread PnL: {pnl:.2f}")
    print(f"Final Portfolio Vega: {Vpi:.2f}")
    
    # 4. Plotting
    os.makedirs("results/plots", exist_ok=True)
    
    plt.figure(figsize=(10, 6))
    plt.plot(times, pnl_history, label="Cumulative Captured Spread (PnL)", color='green')
    plt.title("Historical Backtest: Profit and Loss")
    plt.xlabel("Time (t)")
    plt.ylabel("PnL")
    plt.grid(True, linestyle='--', alpha=0.5)
    plt.legend()
    plt.savefig("results/plots/backtest_pnl.png", dpi=300)
    plt.close()
    
    plt.figure(figsize=(10, 6))
    plt.plot(times, vpi_history, label=r"Portfolio Vega ($V^\pi$)", color='orange')
    plt.title("Historical Backtest: Inventory Risk Over Time")
    plt.xlabel("Time (t)")
    plt.ylabel("Portfolio Vega")
    plt.grid(True, linestyle='--', alpha=0.5)
    plt.legend()
    plt.savefig("results/plots/backtest_vpi.png", dpi=300)
    plt.close()
    
    print("Backtest charts saved to results/plots/backtest_pnl.png and backtest_vpi.png")

if __name__ == "__main__":
    run_backtest()
