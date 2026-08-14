import pandas as pd
import numpy as np
import os

def generate_mock_data():
    np.random.seed(42)
    
    # Market parameters
    S0 = 10.0
    nu0 = 0.0225
    kappa_P = 2.0
    theta_P = 0.04
    xi = 0.2
    rho = -0.5
    
    # Options grid from the paper (20 options)
    strikes = [8.0, 9.0, 10.0, 11.0, 12.0]
    maturities = [1.0, 1.5, 2.0, 3.0]
    
    # Approximate prices, vegas, and z from earlier runs
    # To keep it simple, we load the initial state from quotes_vs_Vpi.csv
    quotes_df = pd.read_csv("results/quotes_vs_Vpi.csv")
    first_vpi = quotes_df['Vpi'].iloc[0]
    initial_options = quotes_df[quotes_df['Vpi'] == first_vpi].copy().reset_index(drop=True)
    
    # Simulation settings
    T = 0.0012
    N_steps = 10000
    dt = T / N_steps
    
    time_series = []
    
    S = S0
    nu = nu0
    
    print(f"Generating {N_steps} ticks of mock market data...")
    
    for step in range(N_steps):
        t = step * dt
        
        # Evolve Heston dynamics (Euler-Maruyama)
        dW1 = np.random.normal(0, np.sqrt(dt))
        dW2 = np.random.normal(0, np.sqrt(dt))
        dWs = rho * dW1 + np.sqrt(1 - rho**2) * dW2
        dWnu = dW1
        
        S = S + np.sqrt(nu) * S * dWs
        nu = nu + kappa_P * (theta_P - nu) * dt + xi * np.sqrt(max(nu, 0)) * dWnu
        nu = max(nu, 1e-8)
        
        # Simulate random market trades for each option
        # We assume base market arrival rate lambda_market
        # If a trade arrives, it has a randomly distributed spread tolerance (delta_market)
        for idx, row in initial_options.iterrows():
            # Adjust the baseline arrival rate to get more trades for backtesting (e.g., multiply by 10)
            z_i = 500000.0 / row['price']
            lambda_i = 10.0 * 252.0 * 30.0 / (1.0 + 0.7 * abs(S0 - row['K']))
            
            # Probability of a market buy (someone pays ask) or market sell (someone hits bid)
            # We assume a market trade arrives with rate lambda_i * 2
            prob_trade = (lambda_i * 2.0) * dt
            
            if np.random.rand() < prob_trade:
                trade_dir = np.random.choice([1, -1])  # 1 = Market Buy (we sell), -1 = Market Sell (we buy)
                
                # The market participant is willing to cross a certain spread.
                # We model delta_market as an exponentially distributed value.
                # A larger spread tolerance means they are more aggressive.
                # Average spread tolerance roughly matches the scale of optimal spreads.
                avg_spread = 0.02
                delta_market = np.random.exponential(scale=avg_spread)
                
                # Current Option Mid Price (Constant Vega Approx)
                mid_price = row['price'] + row['vega'] * (np.sqrt(nu) - np.sqrt(nu0))
                
                if trade_dir == 1:
                    trade_price = mid_price + delta_market
                else:
                    trade_price = mid_price - delta_market
                    
                time_series.append({
                    'time': t,
                    'option_idx': idx,
                    'nu': nu,
                    'mid_price': mid_price,
                    'trade_dir': trade_dir,
                    'trade_price': trade_price,
                    'z_i': z_i,
                    'vega_i': row['vega']
                })
                
    df_market = pd.DataFrame(time_series)
    
    os.makedirs("data", exist_ok=True)
    df_market.to_csv("data/historical_data.csv", index=False)
    print(f"Generated {len(df_market)} market trades and saved to data/historical_data.csv")

if __name__ == "__main__":
    generate_mock_data()
