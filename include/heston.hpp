#pragma once
#include <stdexcept>

using namespace std;

struct HestonParams{
    double S0     = 10.0; 
    double nu0    = 0.0225;
    double kappa_P = 2.0;
    double theta_P = 0.04;
    double kappa_Q = 3.0;
    double theta_Q = 0.0225;
    double xi     = 0.2;
    double rho    = -0.5;

    HestonParams(){
        validate();
    }

    // Drift under P-measure: a^P(t, ν) = κ^P(θ^P − ν)
    double drift_P(double /*t*/, double nu) const {
        return kappa_P * (theta_P - nu);
    }
    
    // Drift under Q-measure: a^Q(t, ν) = κ^Q(θ^Q − ν)
    double drift_Q(double /*t*/, double nu) const {
        return kappa_Q * (theta_Q - nu);
    }

private:
    void validate() const {
        if (2.0 * kappa_P * theta_P <= xi * xi) {
            throw invalid_argument("Feller condition violated under P-measure: 2*kappa*theta <= xi^2");
        }
        
        if (2.0 * kappa_Q * theta_Q <= xi * xi) {
            throw invalid_argument("Feller condition violated under Q-measure: 2*kappa*theta <= xi^2");
        }

        if (nu0 <= 0.0 || S0 <= 0.0) {
            throw invalid_argument("Initial stock price and variance must be strictly positive.");
        }
        
        if (rho < -1.0 || rho > 1.0) {
            throw invalid_argument("Correlation rho must be bounded in [-1, 1].");
        }
    }

};