#pragma once
#include "hjb_solver.hpp"
#include <vector>

using namespace std;

struct QuoteResult {
    double delta_bid;    // Optimal mid-to-bid spread
    double delta_ask;    // Optimal ask-to-mid spread
};

class QuoteEngine {
public:
    QuoteEngine(const HJBSolver& solver, const vector<OptionData>& options);
    
    // Compute optimal quotes for option i at a specific state (t, ν, V^π)
    QuoteResult compute_quote(int option_idx, int t_idx, double nu, double Vpi) const;
    
    // Compute all quotes for all options at a specific state
    vector<QuoteResult> compute_all_quotes(int t_idx, double nu, double Vpi) const;

private:
    const HJBSolver& solver_;
    const vector<OptionData>& options_;
};