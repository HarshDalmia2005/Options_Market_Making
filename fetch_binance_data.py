import requests
import pandas as pd
import numpy as np
import os
import time

def main():
    print("Fetching Binance Options Exchange Info...")
    try:
        res = requests.get('https://eapi.binance.com/eapi/v1/exchangeInfo')
        info = res.json()
        symbols = info['optionSymbols']
    except Exception as e:
        print(f"Error fetching exchange info: {e}")
        return

    # We will need some metadata from our model (z_i, vega_i)
    # Load them from quotes_vs_Vpi.csv
    try:
        quotes_df = pd.read_csv("results/quotes_vs_Vpi.csv")
        first_vpi = quotes_df['Vpi'].iloc[0]
        initial_options = quotes_df[quotes_df['Vpi'] == first_vpi].copy().reset_index(drop=True)
    except Exception as e:
        print("Could not load quotes_vs_Vpi.csv. Ensure the C++ engine has been run.")
        return

    underlyings = ['BTCUSDT', 'ETHUSDT', 'BNBUSDT', 'SOLUSDT']

    for underlying in underlyings:
        print(f"\n======================================")
        print(f"Processing {underlying}...")
        
        # Fetch Spot Price
        try:
            res = requests.get(f'https://api.binance.com/api/v3/ticker/price?symbol={underlying}')
            S_spot = float(res.json()['price'])
            print(f"Current {underlying} Spot: ${S_spot:.2f}")
        except Exception as e:
            print(f"Error fetching spot for {underlying}: {e}")
            continue

        # Filter for active CALL options
        active_calls = [s for s in symbols if s['underlying'] == underlying and s['side'] == 'CALL' and s['status'] == 'TRADING']
        
        # Let's find options near ATM (0.9 to 1.1 moneyness)
        atm_calls = []
        for s in active_calls:
            strike = float(s['strikePrice'])
            moneyness = strike / S_spot
            if 0.8 <= moneyness <= 1.2:
                atm_calls.append(s)

        # Sort by some proxy for activity (we'll just pick a few evenly distributed strikes)
        if len(atm_calls) > 5:
            atm_calls = atm_calls[:10]  # Just take the first 10 for speed

        print(f"Selected {len(atm_calls)} ATM options to fetch trades for.")
        
        all_trades = []

        for s in atm_calls:
            sym = s['symbol']
            strike_actual = float(s['strikePrice'])
            print(f"Fetching trades for {sym}...")
            
            try:
                res = requests.get(f"https://eapi.binance.com/eapi/v1/trades?symbol={sym}&limit=1000")
                trades = res.json()
            except:
                continue
                
            if not isinstance(trades, list) or len(trades) == 0:
                continue
                
            # Normalize strike to our model grid S0=10
            K_norm = 10.0 * (strike_actual / S_spot)
            
            # Find closest grid strike {8, 9, 10, 11, 12}
            grid_strikes = np.array([8.0, 9.0, 10.0, 11.0, 12.0])
            closest_K = grid_strikes[np.abs(grid_strikes - K_norm).argmin()]
            
            # Map to option_idx (assuming T=1.0 which correspond to indices 0, 1, 2, 3, 4 in our C++ model)
            # We can look up the exact option_idx from initial_options
            opt_match = initial_options[(initial_options['K'] == closest_K) & (initial_options['T'] == 1.0)]
            if opt_match.empty:
                continue
            opt_idx = opt_match['option_idx'].values[0]
            vega_i = opt_match['vega'].values[0]
            
            # Calculate EMA of prices to act as Mid Price
            prices = [float(t['price']) for t in trades]
            df_p = pd.DataFrame({'price': prices})
            df_p['mid_ema'] = df_p['price'].ewm(span=10).mean()
            
            for i, t in enumerate(trades):
                price_actual = float(t['price'])
                qty_actual = float(t['qty'])
                price_norm = price_actual * (10.0 / S_spot)
                mid_ema_norm = df_p['mid_ema'].iloc[i] * (10.0 / S_spot)
                trade_dir = t['side']
                
                # Scale quantity to model's z_i
                z_i = opt_match['price'].values[0] > 0 and 500000.0 / opt_match['price'].values[0] or 1000.0
                
                all_trades.append({
                    'time': t['time'] / 1000.0, # convert ms to sec
                    'option_idx': opt_idx,
                    'nu': 0.0225, # Fixed initial variance for simplicity
                    'mid_price': mid_ema_norm,
                    'trade_dir': trade_dir,
                    'trade_price': price_norm,
                    'z_i': z_i,
                    'vega_i': vega_i,
                    'qty_actual': qty_actual,
                    'S_spot': S_spot
                })
                
            time.sleep(0.5) # respect rate limits

        if len(all_trades) == 0:
            print(f"No trades found for {underlying}.")
            continue
            
        df_out = pd.DataFrame(all_trades)
        
        # Sort chronologically
        df_out = df_out.sort_values('time').reset_index(drop=True)
        
        os.makedirs("data", exist_ok=True)
        file_path = f"data/binance_{underlying}.csv"
        df_out.to_csv(file_path, index=False)
        
        print(f"Success! Saved {len(df_out)} real Binance trades to {file_path}")

if __name__ == "__main__":
    main()
