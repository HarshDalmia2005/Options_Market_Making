#pragma once
#include "heston.hpp"
#include <complex>
#include <vector>

using namespace std;

struct OptionSpec {
    double K;          // Strike
    double T;          // Maturity
    double price;      // Computed price at t=0
    double vega;       // ∂O/∂√ν at t=0
    double implied_vol;// BS implied vol
    double z;          // Transaction size (contracts)
    double lambda;     // Base arrival rate
};

class HestonPricer {
public:
    explicit HestonPricer(const HestonParams& params);
    
    // Price a European call using characteristic function integration
    double call_price(double K, double T) const;
    
    // Vega = ∂O/∂√ν = 2√ν · ∂O/∂ν (finite difference on ν)
    double compute_vega(double K, double T, double d_nu = 1e-5) const;
    
    // Black-Scholes implied volatility (Newton-Raphson)
    double implied_vol(double price, double K, double T) const;
    
    // Build all 20 options with prices, vegas, IVs, trade sizes
    vector<OptionSpec> build_option_grid() const;

private:
    HestonParams params_;
    
    // Heston characteristic function φ(u) = E^Q[exp(iu·log(S_T))]
    complex<double> char_func(complex<double> u, double T) const;
    
    // Gauss-Legendre quadrature (faster & more accurate than adaptive)
    double integrate_call(double K, double T) const;
    
    // Black-Scholes helpers for implied vol Newton-Raphson
    double bs_price(double sigma, double K, double T) const;
    double bs_vega(double sigma, double K, double T) const;
};
