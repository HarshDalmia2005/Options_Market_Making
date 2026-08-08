#include "intensity.hpp"
#include <cmath>
#include <algorithm>
#include <limits>

using namespace std;

IntensityFunction::IntensityFunction(double lambda, double alpha, double beta, double vega)
    : lambda_(lambda), alpha_(alpha), beta_(beta), vega_(vega) {}

double IntensityFunction::operator()(double delta) const {
    return lambda_ / (1.0 + exp(alpha_ + beta_ / vega_ * delta));
}

double IntensityFunction::derivative(double delta) const {
    double e = exp(alpha_ + beta_ / vega_ * delta);
    double denom = (1.0 + e);
    return -lambda_ * (beta_ / vega_) * e / (denom * denom);
}

double IntensityFunction::inverse(double y) const {
    if (y <= 0 || y >= lambda_) return numeric_limits<double>::infinity();
    return vega_ / beta_ * (log(lambda_ / y - 1.0) - alpha_);
}

double IntensityFunction::optimal_delta(double p) const {
    double delta = p + vega_ / beta_; 
    for (int iter = 0; iter < 50; ++iter) {
        double L = (*this)(delta);
        double Lp = derivative(delta);
        
        double f = L + Lp * (delta - p);
        if (abs(f) < 1e-12) break;
        
        delta -= f / (2.0 * Lp); 
    }
    return delta;
}

void IntensityFunction::build_lookup_table(double p_min, double p_max, int n_points) {
    p_min_ = p_min;
    p_max_ = p_max;
    dp_ = (p_max - p_min) / (n_points - 1);
    
    p_grid_.resize(n_points);
    H_table_.resize(n_points);
    H_prime_table_.resize(n_points);
    
    for (int k = 0; k < n_points; ++k) {
        double p = p_min + k * dp_;
        p_grid_[k] = p;
        
        double delta_star = optimal_delta(p);
        double L_star = (*this)(delta_star);
        
        H_table_[k] = L_star * (delta_star - p);
        H_prime_table_[k] = -L_star; 
    }
}

double IntensityFunction::H(double p) const {
    if (p <= p_min_) return H_table_.front();
    if (p >= p_max_) return H_table_.back();
    
    double idx_f = (p - p_min_) / dp_;
    int idx = static_cast<int>(idx_f);
    double frac = idx_f - idx;
    
    return H_table_[idx] * (1.0 - frac) + H_table_[idx + 1] * frac;
}

double IntensityFunction::H_prime(double p) const {
    if (p <= p_min_) return H_prime_table_.front();
    if (p >= p_max_) return H_prime_table_.back();
    
    double idx_f = (p - p_min_) / dp_;
    int idx = static_cast<int>(idx_f);
    double frac = idx_f - idx;
    
    return H_prime_table_[idx] * (1.0 - frac) + H_prime_table_[idx + 1] * frac;
}