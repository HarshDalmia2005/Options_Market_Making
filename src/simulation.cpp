#include "simulation.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>

using namespace std;

Simulator::Simulator(const HestonParams& heston, const HJBSolver& solver,
                     const QuoteEngine& quotes, const vector<OptionData>& options)
    : heston_(heston), solver_(solver), quotes_(quotes), options_(options) {}

vector<SimulationResult> Simulator::run(int n_sims, int n_steps, unsigned seed) const {
    vector<SimulationResult> results(n_sims);
    mt19937 gen(seed);
    normal_distribution<double> norm(0.0, 1.0);
    uniform_real_distribution<double> unif(0.0, 1.0);
    
    double T = solver_.grid().T;
    double dt = T / n_steps;
    double sqrt_dt = sqrt(dt);
    
    for (int sim = 0; sim < n_sims; ++sim) {
        double nu = heston_.nu0;
        double Vpi = 0.0;
        double pnl = 0.0;
        int trades = 0;
        
        results[sim].pnl_path.reserve(n_steps + 1);
        results[sim].Vpi_path.reserve(n_steps + 1);
        
        results[sim].pnl_path.push_back(pnl);
        results[sim].Vpi_path.push_back(Vpi);
        
        for (int step = 0; step < n_steps; ++step) {
            double t = step * dt;
            
            // Map continuous time to HJB grid index
            int t_idx = static_cast<int>(round(t / solver_.grid().dt()));
            t_idx = clamp(t_idx, 0, solver_.grid().n_t);
            
            // Limit nu to solver grid for quotes
            double nu_grid = clamp(nu, solver_.grid().nu_min, solver_.grid().nu_max);
            
            // Get quotes for all options and simulate arrivals
            for (size_t i = 0; i < options_.size(); ++i) {
                const auto& opt = options_[i];
                auto quote = quotes_.compute_quote(static_cast<int>(i), t_idx, nu_grid, Vpi);
                
                // Bid trade (MM buys)
                double lambda_bid = opt.intensity(quote.delta_bid);
                double prob_bid = 1.0 - exp(-lambda_bid * dt);
                if (unif(gen) < prob_bid) {
                    Vpi += opt.z * opt.vega;
                    pnl += opt.z * quote.delta_bid;
                    trades++;
                }
                
                // Ask trade (MM sells)
                double lambda_ask = opt.intensity(quote.delta_ask);
                double prob_ask = 1.0 - exp(-lambda_ask * dt);
                if (unif(gen) < prob_ask) {
                    Vpi -= opt.z * opt.vega;
                    pnl += opt.z * quote.delta_ask;
                    trades++;
                }
            }
            
            // Evolve variance (Euler-Maruyama)
            double dZ = norm(gen);
            double dW_nu = dZ * sqrt_dt;
            
            double drift_P = heston_.drift_P(t, nu);
            double drift_Q = heston_.drift_Q(t, nu);
            
            // MtM vega PnL accumulation
            if (nu > 0.0) {
                double vol_premium = (drift_P - drift_Q) / (2.0 * sqrt(nu));
                pnl += Vpi * vol_premium * dt + Vpi * (heston_.xi / 2.0) * dW_nu;
            }
            
            nu += drift_P * dt + heston_.xi * sqrt(max(nu, 0.0)) * dW_nu;
            nu = max(nu, 1e-8); // Reflection/truncation to prevent negative variance
            
            // Enforce risk limits
            Vpi = clamp(Vpi, -solver_.grid().V_bar, solver_.grid().V_bar);
            
            results[sim].pnl_path.push_back(pnl);
            results[sim].Vpi_path.push_back(Vpi);
        }
        
        results[sim].final_pnl = pnl;
        results[sim].final_Vpi = Vpi;
        results[sim].total_trades = trades;
        
        if ((sim + 1) % max(1, n_sims / 10) == 0) {
            cout << "  Simulation " << (sim + 1) << "/" << n_sims << "\r" << flush;
        }
    }
    cout << "\nSimulations complete.\n";
    
    return results;
}
