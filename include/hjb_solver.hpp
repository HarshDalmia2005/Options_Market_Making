#pragma once
#include "heston.hpp"
#include "intensity.hpp"
#include <vector>

using namespace std;

struct HJBGrid {
    int n_t;
    int n_nu;
    int n_Vpi;
    double T;
    double nu_min, nu_max;
    double V_bar;
    
    double dt() const { return T / n_t; }
    double d_nu() const { return (nu_max - nu_min) / (n_nu - 1); }
    double d_Vpi() const { return 2.0 * V_bar / (n_Vpi - 1); }
    
    double nu(int j) const { return nu_min + j * d_nu(); }
    double Vpi(int k) const { return -V_bar + k * d_Vpi(); }
};

struct OptionData {
    double vega;
    double z;
    IntensityFunction intensity;
};

class HJBSolver {
public:
    HJBSolver(const HestonParams& heston, const HJBGrid& grid, const vector<OptionData>& options, double gamma);
    
    void solve();
    
    double v(int t, int j, int k) const { return v_[idx(t, j, k)]; }
    double v_interp(int t, double nu, double Vpi) const;
    const HJBGrid& grid() const { return grid_; }

private:
    HestonParams heston_;
    HJBGrid grid_;
    vector<OptionData> options_;
    double gamma_;
    vector<double> v_;
    
    int idx(int t, int j, int k) const {
        return t * grid_.n_nu * grid_.n_Vpi + j * grid_.n_Vpi + k;
    }
    
    double interp_Vpi(int t, int j, double Vpi) const;
};