#include "optimal_quotes.hpp"
#include <algorithm>

using namespace std;

QuoteEngine::QuoteEngine(const HJBSolver& solver, const vector<OptionData>& options)
    : solver_(solver), options_(options) {}

QuoteResult QuoteEngine::compute_quote(int i, int t_idx, double nu, double Vpi) const {
    const auto& opt = options_[i];
    double zi = opt.z;
    double Vi = opt.vega;
    
    double v_curr = solver_.v_interp(t_idx, nu, Vpi);
    double v_bar = solver_.grid().V_bar;
    
    QuoteResult result;
    
    // Bid side (psi = -1): Market maker buys, portfolio vega increases by zi * Vi
    {
        double Vpi_new = Vpi + zi * Vi; 
        double v_new = solver_.v_interp(t_idx, nu, clamp(Vpi_new, -v_bar, v_bar));
        double p = (v_curr - v_new) / zi;
        
        double neg_Hp = -opt.intensity.H_prime(p);
        result.delta_bid = opt.intensity.inverse(neg_Hp);
    }
    
    // Ask side (psi = +1): Market maker sells, portfolio vega decreases by zi * Vi
    {
        double Vpi_new = Vpi - zi * Vi; 
        double v_new = solver_.v_interp(t_idx, nu, clamp(Vpi_new, -v_bar, v_bar));
        double p = (v_curr - v_new) / zi;
        
        double neg_Hp = -opt.intensity.H_prime(p);
        result.delta_ask = opt.intensity.inverse(neg_Hp);
    }
    
    return result;
}

vector<QuoteResult> QuoteEngine::compute_all_quotes(int t_idx, double nu, double Vpi) const {
    vector<QuoteResult> results;
    results.reserve(options_.size());
    for (int i = 0; i < static_cast<int>(options_.size()); ++i) {
        results.push_back(compute_quote(i, t_idx, nu, Vpi));
    }
    return results;
}