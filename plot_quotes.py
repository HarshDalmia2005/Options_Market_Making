import pandas as pd
import matplotlib.pyplot as plt
import os

def main():
    plot_dir = "results/plots"
    os.makedirs(plot_dir, exist_ok=True)
    
    # Prefix for reading data
    prefix = "results/" if os.path.exists("results/") else "../results/"
    
    # ---------------------------------------------------------
    # PLOT 1: Value Function (Figure 2)
    # ---------------------------------------------------------
    val_path = prefix + "value_function.csv"
    if os.path.exists(val_path):
        print(f"Loading {val_path}...")
        df_v = pd.read_csv(val_path)
        
        plt.figure(figsize=(10, 6))
        # Plot for 3 distinct values of nu (low, mid, high)
        nus = sorted(df_v['nu'].unique())
        if len(nus) > 0:
            selected_nus = [nus[0], nus[len(nus)//2], nus[-1]]
            for nu in selected_nus:
                df_nu = df_v[df_v['nu'] == nu]
                plt.plot(df_nu['Vpi'], df_nu['v'], label=f"nu = {nu:.4f}")
            
            plt.title(r"Value Function $v(0, \nu, V^\pi)$")
            plt.xlabel(r"Portfolio vega $V^\pi$")
            plt.ylabel("Value Function $v$")
            plt.legend()
            plt.grid(True, linestyle='--', alpha=0.5)
            plt.savefig(os.path.join(plot_dir, "value_function.png"), dpi=300)
            plt.close()
    else:
        print(f"Warning: {val_path} not found.")

    # ---------------------------------------------------------
    # PLOT 2 & 3: Optimal Quotes (Figures 4-13)
    # ---------------------------------------------------------
    quotes_path = prefix + "quotes_vs_Vpi.csv"
    if os.path.exists(quotes_path):
        print(f"Loading {quotes_path}...")
        df_q = pd.read_csv(quotes_path)
        
        strikes = sorted(df_q['K'].unique())
        maturities = sorted(df_q['T'].unique())

        print("Generating quotes plots...")
        for K in strikes:
            df_k = df_q[df_q['K'] == K]
            
            # Optimal mid-to-bid / price
            plt.figure(figsize=(10, 6))
            for T in maturities:
                df_t = df_k[df_k['T'] == T]
                if not df_t.empty:
                    opt_price = df_t['price'].iloc[0]
                    opt_vega = df_t['vega'].iloc[0]
                    label = f"T={T} (P={opt_price:.2f}, V={opt_vega:.2f})"
                    plt.plot(df_t['Vpi'], df_t['delta_bid_over_price'], marker='.', linestyle='-', label=label)
            
            plt.title(f"Optimal mid-to-bid spread over price (K={K})")
            plt.xlabel(r"Portfolio vega $V^\pi$")
            plt.ylabel(r"$\delta^{i,b} / O^i$")
            plt.legend()
            plt.grid(True, linestyle='--', alpha=0.5)
            plt.savefig(os.path.join(plot_dir, f"mid_to_bid_K_{K}.png"), dpi=300)
            plt.close()

            # Bid IV / Initial IV
            plt.figure(figsize=(10, 6))
            for T in maturities:
                df_t = df_k[df_k['T'] == T]
                if not df_t.empty:
                    opt_iv = df_t['iv_initial'].iloc[0]
                    label = f"T={T} (IV0={opt_iv:.4f})"
                    plt.plot(df_t['Vpi'], df_t['iv_bid_over_iv0'], marker='+', linestyle='--', label=label)
            
            plt.title(f"IV of optimal bid quote over initial IV (K={K})")
            plt.xlabel(r"Portfolio vega $V^\pi$")
            plt.ylabel("Bid IV / Initial IV")
            plt.legend()
            plt.grid(True, linestyle='--', alpha=0.5)
            plt.savefig(os.path.join(plot_dir, f"iv_ratio_K_{K}.png"), dpi=300)
            plt.close()
    else:
        print(f"Warning: {quotes_path} not found.")

    # ---------------------------------------------------------
    # PLOT 4 & 5: Simulation Results
    # ---------------------------------------------------------
    sim_path = prefix + "simulation_summary.csv"
    if os.path.exists(sim_path):
        print(f"Loading {sim_path}...")
        df_s = pd.read_csv(sim_path)
        
        # PnL Histogram
        plt.figure(figsize=(10, 6))
        plt.hist(df_s['final_pnl'], bins=30, edgecolor='black', alpha=0.7)
        plt.title("Distribution of Final PnL")
        plt.xlabel("Final PnL")
        plt.ylabel("Frequency")
        plt.grid(True, linestyle='--', alpha=0.5)
        plt.savefig(os.path.join(plot_dir, "pnl_histogram.png"), dpi=300)
        plt.close()
        
        # PnL vs Final Vpi
        plt.figure(figsize=(10, 6))
        plt.scatter(df_s['final_Vpi'], df_s['final_pnl'], alpha=0.5, c=df_s['total_trades'], cmap='viridis')
        plt.colorbar(label='Total Trades')
        plt.title("Final PnL vs Final Portfolio Vega")
        plt.xlabel(r"Final Portfolio Vega $V^\pi$")
        plt.ylabel("Final PnL")
        plt.grid(True, linestyle='--', alpha=0.5)
        plt.savefig(os.path.join(plot_dir, "pnl_vs_vega.png"), dpi=300)
        plt.close()
    else:
        print(f"Warning: {sim_path} not found.")

    print(f"Success! All charts have been saved to the '{plot_dir}' directory.")

if __name__ == "__main__":
    main()