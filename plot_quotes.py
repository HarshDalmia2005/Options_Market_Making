import pandas as pd
import matplotlib.pyplot as plt
import os

def main():
    # Locate the CSV file whether run from root or the scripts folder
    csv_path = "results/quotes_vs_Vpi.csv"
    if not os.path.exists(csv_path):
        csv_path = "../results/quotes_vs_Vpi.csv"
        
    if not os.path.exists(csv_path):
        print(f"Error: Could not find {csv_path}")
        return

    print("Loading data...")
    df = pd.read_csv(csv_path)
    
    strikes = sorted(df['K'].unique())
    maturities = sorted(df['T'].unique())

    # Create a directory to save the plots
    plot_dir = "results/plots"
    os.makedirs(plot_dir, exist_ok=True)

    print("Generating plots...")
    for K in strikes:
        df_k = df[df['K'] == K]
        
        # ---------------------------------------------------------
        # PLOT 1: Optimal mid-to-bid divided by price (Figs 4-8)
        # ---------------------------------------------------------
        plt.figure(figsize=(10, 6))
        for T in maturities:
            df_t = df_k[df_k['T'] == T]
            
            # Extract a single representative price and vega for the legend
            opt_price = df_t['price'].iloc[0]
            opt_vega = df_t['vega'].iloc[0]
            
            label = f"(K,T)=({K},{T}) - price={opt_price:.2f}, vega={opt_vega:.2f}"
            plt.plot(df_t['Vpi'], df_t['delta_bid_over_price'], marker='.', linestyle='-', label=label)
        
        plt.title(f"Optimal mid-to-bid divided by price (K={K})")
        plt.xlabel("Portfolio vega")
        plt.ylabel("Optimal mid-to-bid divided by price")
        plt.legend()
        plt.grid(True, linestyle='--', alpha=0.5)
        
        out_path_1 = os.path.join(plot_dir, f"mid_to_bid_K_{K}.png")
        plt.savefig(out_path_1, dpi=300)
        plt.close()

        # ---------------------------------------------------------
        # PLOT 2: Bid IV divided by Initial IV (Figs 9-13)
        # ---------------------------------------------------------
        plt.figure(figsize=(10, 6))
        for T in maturities:
            df_t = df_k[df_k['T'] == T]
            
            opt_price = df_t['price'].iloc[0]
            opt_vega = df_t['vega'].iloc[0]
            opt_iv = df_t['iv_initial'].iloc[0]
            
            label = f"(K,T)=({K},{T}) - price={opt_price:.2f}, vega={opt_vega:.2f}, IV={opt_iv:.4f}"
            plt.plot(df_t['Vpi'], df_t['iv_bid_over_iv0'], marker='+', linestyle='--', label=label)
        
        plt.title(f"IV of optimal bid quote divided by initial IV (K={K})")
        plt.xlabel("Portfolio vega")
        plt.ylabel("IV of optimal bid quote divided by initial IV")
        plt.legend()
        plt.grid(True, linestyle='--', alpha=0.5)
        
        out_path_2 = os.path.join(plot_dir, f"iv_ratio_K_{K}.png")
        plt.savefig(out_path_2, dpi=300)
        plt.close()

    print(f"Success! 10 charts have been saved to the '{plot_dir}' directory.")

if __name__ == "__main__":
    main()