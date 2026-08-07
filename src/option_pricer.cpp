#include "option_pricer.hpp"
#include <cmath>
#include <iostream>

using namespace std;
using cd = complex<double>;

const double PI = acos(-1.0);

HestonPricer::HestonPricer(const HestonParams& params) : params_(params) {}

// Standard Heston characteristic function: E^Q[exp(i*u*ln(S_T))]
// Uses the formulation from Gatheral (2006) / Kahl & Jäckel (2005)
// with the sign convention that avoids branch-cut discontinuities.
//
// Under Q: dS = sqrt(v)*S*dW^S, dv = kappa_Q*(theta_Q - v)*dt + xi*sqrt(v)*dW^v
// with corr(W^S, W^v) = rho
cd HestonPricer::char_func(cd u, double T) const {
    const auto& p = params_;
    const cd I(0.0, 1.0);
    
    cd iu = I * u;
    
    // Standard Heston formulation
    cd d = sqrt(
        (p.rho * p.xi * iu - p.kappa_Q) * (p.rho * p.xi * iu - p.kappa_Q)
        + p.xi * p.xi * (iu + u * u)
    );
    
    // Lord & Kahl: choose sign so Re(d) > 0 for stability
    if (real(d) < 0.0) d = -d;
    
    // g ratio (Form 2 - numerically stable for large T)
    cd g_minus = p.kappa_Q - p.rho * p.xi * iu - d;
    cd g_plus  = p.kappa_Q - p.rho * p.xi * iu + d;
    cd g = g_minus / g_plus;
    
    cd exp_dT = exp(-d * T);
    
    // C and D coefficients
    cd D = g_minus / (p.xi * p.xi) * (1.0 - exp_dT) / (1.0 - g * exp_dT);
    
    cd C = p.kappa_Q * p.theta_Q / (p.xi * p.xi) * (
        g_minus * T - 2.0 * log((1.0 - g * exp_dT) / (1.0 - g))
    );
    
    return exp(C + D * p.nu0 + iu * log(p.S0));
}

// Gil-Pelaez inversion: compute P1 and P2 such that Call = S*P1 - K*P2
// P_j = 0.5 + 1/π ∫₀^∞ Re[e^{-iu·ln(K)} φ_j(u) / (iu)] du
// where φ_1(u) = φ(u-i)/φ(-i) and φ_2(u) = φ(u)
// and φ(u) = char_func(u) is E^Q[exp(iu·ln(S_T))]
double HestonPricer::call_price(double K, double T) const {
    int N = 4000;
    double u_max = 200.0;
    double du = u_max / N;
    double logK = log(K);
    
    double integral_P1 = 0.0;
    double integral_P2 = 0.0;
    
    // Simpson's rule
    for (int k = 1; k <= N; ++k) {
        double u = k * du;
        cd iu = cd(0.0, u);
        
        cd phi = char_func(cd(u, 0.0), T);  // φ(u)
        
        // P2 integrand: Re[e^{-iu·ln(K)} · φ(u) / (iu)]
        cd integrand2 = exp(-iu * logK) * phi / iu;
        double f2 = real(integrand2);
        
        // P1 integrand: Re[e^{-iu·ln(K)} · φ(u-i) / (iu · φ(-i))]
        // φ(-i) = E[S_T/S_0] = 1 (under Q with r=0, S is a martingale)
        // So φ_1(u) = φ(u-i) / S_0  (since φ(-i) = S_0 when using ln(S_T))
        // Actually: φ_1(u) = φ(u - i) / φ(-i), and φ(-i) = E[exp(ln(S_T))] = E[S_T] = S_0
        cd phi_shifted = char_func(cd(u, -1.0), T);  // φ(u - i)
        cd integrand1 = exp(-iu * logK) * phi_shifted / (iu * params_.S0);
        double f1 = real(integrand1);
        
        if (isnan(f1) || isinf(f1)) f1 = 0.0;
        if (isnan(f2) || isinf(f2)) f2 = 0.0;
        
        double weight;
        if (k == N) {
            weight = 1.0;  // endpoint
        } else {
            weight = (k % 2 == 1) ? 4.0 : 2.0;
        }
        
        integral_P1 += weight * f1;
        integral_P2 += weight * f2;
    }
    
    integral_P1 *= du / 3.0;
    integral_P2 *= du / 3.0;
    
    double P1 = 0.5 + integral_P1 / PI;
    double P2 = 0.5 + integral_P2 / PI;
    
    return params_.S0 * P1 - K * P2;
}

// Kept for interface compatibility but now unused internally
double HestonPricer::integrate_call(double K, double T) const {
    return call_price(K, T);
}

double HestonPricer::compute_vega(double K, double T, double d_nu) const {
    HestonParams params_up = params_;
    params_up.nu0 += d_nu;
    HestonPricer pricer_up(params_up);
    
    HestonParams params_down = params_;
    params_down.nu0 -= d_nu;
    HestonPricer pricer_down(params_down);
    
    double price_up = pricer_up.call_price(K, T);
    double price_down = pricer_down.call_price(K, T);
    
    double dO_dnu = (price_up - price_down) / (2.0 * d_nu);
    return 2.0 * sqrt(params_.nu0) * dO_dnu;
}

double HestonPricer::bs_price(double sigma, double K, double T) const {
    double d1 = (log(params_.S0 / K) + 0.5 * sigma * sigma * T) / (sigma * sqrt(T));
    double d2 = d1 - sigma * sqrt(T);
    return params_.S0 * 0.5 * erfc(-d1 / sqrt(2.0)) - K * 0.5 * erfc(-d2 / sqrt(2.0));
}

double HestonPricer::bs_vega(double sigma, double K, double T) const {
    double d1 = (log(params_.S0 / K) + 0.5 * sigma * sigma * T) / (sigma * sqrt(T));
    return params_.S0 * sqrt(T) * exp(-0.5 * d1 * d1) / sqrt(2.0 * PI);
}

double HestonPricer::implied_vol(double price, double K, double T) const {
    double sigma = 0.2;
    for (int i = 0; i < 100; ++i) {
        double diff = bs_price(sigma, K, T) - price;
        if (fabs(diff) < 1e-8) break;
        
        double v = bs_vega(sigma, K, T);
        if (v < 1e-12) break;
        
        sigma -= diff / v;
        if (sigma <= 0.001) sigma = 0.001;
        if (sigma > 5.0) sigma = 5.0;
    }
    return sigma;
}

vector<OptionSpec> HestonPricer::build_option_grid() const {
    vector<OptionSpec> grid;
    vector<double> strikes = {8.0, 9.0, 10.0, 11.0, 12.0};
    vector<double> maturities = {1.0, 1.5, 2.0, 3.0};
    
    for (double K : strikes) {
        for (double T : maturities) {
            double price = call_price(K, T);
            double vega = compute_vega(K, T);
            double iv = implied_vol(price, K, T);
            
            double z = 500000.0 / price;
            double lambda = 252.0 * 30.0 / (1.0 + 0.7 * fabs(params_.S0 - K));
            
            grid.push_back({K, T, price, vega, iv, z, lambda});
        }
    }
    return grid;
}