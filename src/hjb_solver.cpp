#include "hjb_solver.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>

using namespace std;

HJBSolver::HJBSolver(const HestonParams& heston, const HJBGrid& grid, const vector<OptionData>& options, double gamma)
    : heston_(heston), grid_(grid), options_(options), gamma_(gamma) {
    v_.resize((grid_.n_t + 1) * grid_.n_nu * grid_.n_Vpi, 0.0);
}

double HJBSolver::interp_Vpi(int t, int j, double Vpi) const {
    double Vpi_min = -grid_.V_bar;
    double dV = grid_.d_Vpi();
    
    double idx_f = (Vpi - Vpi_min) / dV;
    idx_f = clamp(idx_f, 0.0, static_cast<double>(grid_.n_Vpi - 1));
    
    int k0 = static_cast<int>(idx_f);
    int k1 = min(k0 + 1, grid_.n_Vpi - 1);
    double frac = idx_f - k0;
    
    return v(t, j, k0) * (1.0 - frac) + v(t, j, k1) * frac;
}

double HJBSolver::v_interp(int t, double nu_val, double Vpi_val) const {
    double dnu = grid_.d_nu();
    double idx_nu = (nu_val - grid_.nu_min) / dnu;
    idx_nu = clamp(idx_nu, 0.0, static_cast<double>(grid_.n_nu - 1));
    
    int j0 = static_cast<int>(idx_nu);
    int j1 = min(j0 + 1, grid_.n_nu - 1);
    double frac_nu = idx_nu - j0;
    
    double v0 = interp_Vpi(t, j0, Vpi_val);
    double v1 = interp_Vpi(t, j1, Vpi_val);
    
    return v0 * (1.0 - frac_nu) + v1 * frac_nu;
}

void HJBSolver::solve() {
    const double dt = grid_.dt();
    const double dnu = grid_.d_nu();
    const double xi2 = heston_.xi * heston_.xi;
    const int N = static_cast<int>(options_.size());
    
    for (int ti = grid_.n_t - 1; ti >= 0; --ti) {
        double t = ti * dt;
        
        for (int j = 0; j < grid_.n_nu; ++j) {
            double nu = grid_.nu(j);
            double sqrt_nu = sqrt(nu);
            double aP = heston_.drift_P(t, nu);
            double aQ = heston_.drift_Q(t, nu);
            
            for (int k = 0; k < grid_.n_Vpi; ++k) {
                double Vpi = grid_.Vpi(k);
                double dv_dnu, d2v_dnu2;
                
                if (j == 0) {
                    dv_dnu = (v(ti + 1, 1, k) - v(ti + 1, 0, k)) / dnu;
                    d2v_dnu2 = 0.0;
                } else if (j == grid_.n_nu - 1) {
                    dv_dnu = (v(ti + 1, j, k) - v(ti + 1, j - 1, k)) / dnu;
                    d2v_dnu2 = 0.0;
                } else {
                    dv_dnu = (v(ti + 1, j + 1, k) - v(ti + 1, j - 1, k)) / (2.0 * dnu);
                    d2v_dnu2 = (v(ti + 1, j + 1, k) - 2.0 * v(ti + 1, j, k) + v(ti + 1, j - 1, k)) / (dnu * dnu);
                }
                
                double diffusion = aP * dv_dnu + 0.5 * nu * xi2 * d2v_dnu2;
                double vol_premium = Vpi * (aP - aQ) / (2.0 * sqrt_nu);
                double risk_penalty = -gamma_ * xi2 / 8.0 * Vpi * Vpi;
                double jump_term = 0.0;
                
                for (int i = 0; i < N; ++i) {
                    const auto& opt = options_[i];
                    double Vi = opt.vega;
                    double zi = opt.z;
                    
                    for (int side = 0; side < 2; ++side) {
                        double psi = (side == 0) ? -1.0 : 1.0; 
                        double Vpi_new = Vpi - psi * zi * Vi;
                        
                        if (abs(Vpi_new) > grid_.V_bar) continue;
                        
                        double v_shifted = interp_Vpi(ti + 1, j, Vpi_new);
                        double v_curr = v(ti + 1, j, k);
                        double p = (v_curr - v_shifted) / zi;
                        
                        jump_term += zi * opt.intensity.H(p);
                    }
                }
                
                v_[idx(ti, j, k)] = v(ti + 1, j, k) + dt * (diffusion + vol_premium + risk_penalty + jump_term);
            }
        }
    }
}