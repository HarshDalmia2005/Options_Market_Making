#pragma once
#include <vector>

using namespace std;

class IntensityFunction {
public:
    IntensityFunction(double lambda, double alpha, double beta, double vega);
    
    double operator()(double delta) const;
    double derivative(double delta) const;
    double inverse(double y) const;
    
    void build_lookup_table(double p_min, double p_max, int n_points);

    double H(double p) const;
    double H_prime(double p) const;

private:
    double lambda_, alpha_, beta_, vega_;
    
    vector<double> p_grid_;
    vector<double> H_table_;
    vector<double> H_prime_table_;
    double p_min_, p_max_, dp_;
    
    double optimal_delta(double p) const;
};