#pragma once
#include "hjb_solver.hpp"
#include "optimal_quotes.hpp"
#include <random>
#include <vector>

struct SimulationResult {
    double final_pnl;
    double final_Vpi;
    int total_trades;
    std::vector<double> pnl_path;    // MtM value over time
    std::vector<double> Vpi_path;    // portfolio vega over time
};

class Simulator {
public:
    Simulator(const HestonParams& heston, const HJBSolver& solver,
              const QuoteEngine& quotes, const std::vector<OptionData>& options);
    
    // Run n_sims simulations, each over [0, T] with n_steps
    std::vector<SimulationResult> run(int n_sims, int n_steps, 
                                      unsigned seed = 42) const;

private:
    HestonParams heston_;
    const HJBSolver& solver_;
    const QuoteEngine& quotes_;
    const std::vector<OptionData>& options_;
};
