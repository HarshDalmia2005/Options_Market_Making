#include "heston.hpp"
#include "option_pricer.hpp"
#include "intensity.hpp"
#include "hjb_solver.hpp"
#include "optimal_quotes.hpp"
#include "simulation.hpp"
#include <iostream>
#include <iomanip>
#include <vector>
#include <chrono>
#include <fstream>
#include <filesystem>

using namespace std;
namespace fs = filesystem;

int main() {
    try {
        cout << "========================================================\n";
        cout << " INITIALIZING MARKET MAKER ENGINE                       \n";
        cout << "========================================================\n";
        
        HestonParams heston;
        HestonPricer pricer(heston);
        vector<OptionSpec> options_spec = pricer.build_option_grid();
        
        double alpha = 0.7;
        double beta = 150.0;
        
        vector<OptionData> options;
        for (const auto& o : options_spec) {
            IntensityFunction intensity(o.lambda, alpha, beta, o.vega);
            intensity.build_lookup_table(-5.0, 5.0, 2000);
            options.push_back({o.vega, o.z, move(intensity)});
        }
        
        HJBGrid grid;
        grid.n_t = 180;
        grid.n_nu = 30;
        grid.n_Vpi = 40;
        grid.T = 0.0012; 
        grid.nu_min = 0.0144;
        grid.nu_max = 0.0324;
        grid.V_bar = 1e7;
        
        double gamma = 1e-3;
        
        cout << "Solving 3D HJB PDE (180x30x40 grid)...\n";
        auto t_start = chrono::high_resolution_clock::now();
        
        HJBSolver solver(heston, grid, options, gamma);
        solver.solve();
        
        auto t_end = chrono::high_resolution_clock::now();
        cout << "Solve completed in " << chrono::duration<double>(t_end - t_start).count() << " seconds.\n\n";

        cout << "========================================================\n";
        cout << " EXTRACTING OPTIMAL QUOTES & WRITING TO CSV             \n";
        cout << "========================================================\n";
        
        QuoteEngine quote_engine(solver, options);
        
        // Ensure the results directory exists in the project root
        if (!fs::exists("../results")) {
            fs::create_directory("../results");
        }
        
        ofstream csv("../results/quotes_vs_Vpi.csv");
        csv << "option_idx,K,T,Vpi,price,vega,iv_initial,delta_bid,delta_ask,delta_bid_over_price,iv_bid_over_iv0\n";
        
        // Fix variance to the initial value for the output curves
        double nu_fixed = heston.nu0; 
        
        for (int k = 0; k < grid.n_Vpi; ++k) {
            double Vpi = grid.Vpi(k);
            auto all_quotes = quote_engine.compute_all_quotes(0, nu_fixed, Vpi);
            
            for (int i = 0; i < static_cast<int>(options_spec.size()); ++i) {
                const auto& spec = options_spec[i];
                double delta_bid = all_quotes[i].delta_bid;
                double delta_ask = all_quotes[i].delta_ask;
                
                double bid_price = spec.price - delta_bid;
                
                // Ensure bid price is positive before computing implied volatility
                double bid_iv = spec.implied_vol;
                if (bid_price > 0) {
                    bid_iv = pricer.implied_vol(bid_price, spec.K, spec.T);
                }
                
                csv << i << "," 
                    << spec.K << "," 
                    << spec.T << "," 
                    << Vpi << "," 
                    << spec.price << "," 
                    << spec.vega << "," 
                    << spec.implied_vol << "," 
                    << delta_bid << "," 
                    << delta_ask << "," 
                    << (delta_bid / spec.price) << "," 
                    << (bid_iv / spec.implied_vol) << "\n";
            }
        }
        
        cout << "Success: Data written to ../results/quotes_vs_Vpi.csv\n";
        cout << "(Use this CSV to plot Figures 4-13 with Matplotlib)\n";
        
        cout << "\n========================================================\n";
        cout << " RUNNING MONTE-CARLO SIMULATIONS                        \n";
        cout << "========================================================\n";
        
        Simulator simulator(heston, solver, quote_engine, options);
        int n_sims = 1000;
        int n_steps = 180;
        
        auto sim_start = chrono::high_resolution_clock::now();
        auto sim_results = simulator.run(n_sims, n_steps, 42);
        auto sim_end = chrono::high_resolution_clock::now();
        
        cout << "Simulations completed in " << chrono::duration<double>(sim_end - sim_start).count() << " seconds.\n";
        
        ofstream sim_csv("../results/simulation_summary.csv");
        sim_csv << "sim_id,final_pnl,final_Vpi,total_trades\n";
        for (int i = 0; i < n_sims; ++i) {
            sim_csv << i << ","
                    << sim_results[i].final_pnl << ","
                    << sim_results[i].final_Vpi << ","
                    << sim_results[i].total_trades << "\n";
        }
        cout << "Success: Simulation data written to ../results/simulation_summary.csv\n";

    } catch (const exception& e) {
        cerr << "Fatal Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}