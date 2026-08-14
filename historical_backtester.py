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
    data_path = "data/binance_historical_data.csv"
    if not os.path.exists(data_path):
        print(f"Error: {data_path} not found. Run fetch_binance_data.py first.")
        return
        
    print("Loading historical data...")
    df_market = pd.read_csv(data_path)
    
    # 3. Initialize Portfolio State and Run Backtest
    Vpi = 0.0
    pnl = 0.0
    real_usd_pnl = 0.0
    trades_executed = 0
    pnl_history = []
    real_pnl_history = []
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
            if 'qty_actual' in row and 'S_spot' in row:
                # Market makers don't post infinite size. We cap our fill at MAX_ORDER_SIZE
                MAX_ORDER_SIZE = 0.5 # BTC
                filled_qty = min(row['qty_actual'], MAX_ORDER_SIZE)
                real_usd_pnl += filled_qty * (delta_bid * (row['S_spot'] / 10.0))
            trade_happened = True
        elif trade_dir == 1 and trade_price >= our_ask_price:
            Vpi -= z_i * vega_i
            pnl += z_i * delta_ask
            if 'qty_actual' in row and 'S_spot' in row:
                MAX_ORDER_SIZE = 0.5 # BTC
                filled_qty = min(row['qty_actual'], MAX_ORDER_SIZE)
                real_usd_pnl += filled_qty * (delta_ask * (row['S_spot'] / 10.0))
            trade_happened = True
            
        if trade_happened:
            trades_executed += 1
            
        pnl_history.append(pnl)
        real_pnl_history.append(real_usd_pnl)
        vpi_history.append(Vpi)
        times.append(row['time'])
        
    print(f"Backtest completed! Executed {trades_executed} out of {len(df_market)} market ticks.")
    print(f"Final Captured Spread PnL (Normalized): {pnl:.2f}")
    if 'qty_actual' in df_market.columns:
        print(f"Final Realized Profit (Actual USD): ${real_usd_pnl:.2f}")
    print(f"Final Portfolio Vega: {Vpi:.2f}")
    
    # 4. Plotting
    plot_dir = "results/plots/backtest"
    os.makedirs(plot_dir, exist_ok=True)
    
    plt.figure(figsize=(10, 6))
    plt.plot(times, pnl_history, label="Cumulative Captured Spread (PnL)", color='green')
    plt.title("Historical Backtest: Profit and Loss")
    plt.xlabel("Time (t)")
    plt.ylabel("PnL")
    plt.grid(True, linestyle='--', alpha=0.5)
    plt.legend()
    
    # Save depending on dataset used
    dataset_name = "binance" if "binance" in data_path else "synthetic"
    pnl_plot_path = f"{plot_dir}/backtest_{dataset_name}_pnl.png"
    plt.savefig(pnl_plot_path, dpi=300)
    plt.close()
    
    # Plot 1b: Real USD PnL
    if 'qty_actual' in df_market.columns:
        plt.figure(figsize=(10, 6))
        plt.plot(times, real_pnl_history, label="Realized Profit (Actual USD)", color='purple')
        plt.title("Historical Backtest: Real USD Profit (With Size Limits)")
        plt.xlabel("Time (t)")
        plt.ylabel("USD Profit ($)")
        plt.grid(True, linestyle='--', alpha=0.5)
        plt.legend()
        real_pnl_plot_path = f"{plot_dir}/backtest_{dataset_name}_real_pnl.png"
        plt.savefig(real_pnl_plot_path, dpi=300)
        plt.close()
    
    plt.figure(figsize=(10, 6))
    plt.plot(times, vpi_history, label=r"Portfolio Vega ($V^\pi$)", color='orange')
    plt.title("Historical Backtest: Inventory Risk Over Time")
    plt.xlabel("Time (t)")
    plt.ylabel("Portfolio Vega")
    plt.grid(True, linestyle='--', alpha=0.5)
    plt.legend()
    vpi_plot_path = f"{plot_dir}/backtest_{dataset_name}_vpi.png"
    plt.savefig(vpi_plot_path, dpi=300)
    plt.close()
    
    # New Plot: Actual Market Data Received
    plt.figure(figsize=(10, 6))
    
    # We need the times and mid prices from df_market
    market_times = df_market['time']
    mid_prices = df_market['mid_price']
    
    # Split trades by direction
    buys = df_market[df_market['trade_dir'] == 1]
    sells = df_market[df_market['trade_dir'] == -1]
    
    plt.plot(market_times, mid_prices, label="Market Mid Price (Normalized)", color='blue', alpha=0.5)
    plt.scatter(buys['time'], buys['trade_price'], color='green', marker='^', s=15, label="Market Buys (Hit our Ask)", alpha=0.7)
    plt.scatter(sells['time'], sells['trade_price'], color='red', marker='v', s=15, label="Market Sells (Hit our Bid)", alpha=0.7)
    
    plt.title(f"Actual Market Order Flow ({dataset_name.upper()} - BTCUSDT)")
    plt.xlabel("Time (t)")
    plt.ylabel("Normalized Option Price")
    plt.grid(True, linestyle='--', alpha=0.5)
    plt.legend()
    
    market_plot_path = f"{plot_dir}/backtest_{dataset_name}_market.png"
    plt.savefig(market_plot_path, dpi=300)
    plt.close()
    
    print(f"Backtest charts saved to {plot_dir}")

if __name__ == "__main__":
    run_backtest()
