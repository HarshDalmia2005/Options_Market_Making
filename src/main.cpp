#include "heston.hpp"
#include "option_pricer.hpp"
#include "intensity.hpp"
#include "hjb_solver.hpp"
#include <iostream>
#include <iomanip>
#include <vector>
#include <chrono>

using namespace std;

int main() {
    try {
        cout << "========================================================\n";
        cout << " MODULE 1 & 2: Option Pricing & Greeks (Heston Model)   \n";
        cout << "========================================================\n";
        
        HestonParams heston;
        HestonPricer pricer(heston);
        vector<OptionSpec> options_spec = pricer.build_option_grid();
        
        cout << fixed << setprecision(4);
        cout << " K     T       Price     Vega      IV         Lambda\n";
        cout << "--------------------------------------------------------\n";
        for (const auto& opt : options_spec) {
            // Displaying only T=1.0 for terminal readability
            if (opt.T == 1.0) {
                cout << setw(4) << setprecision(1) << opt.K << "  "
                     << setw(4) << opt.T << "  "
                     << setw(8) << setprecision(2) << opt.price << "  "
                     << setw(8) << opt.vega << "  "
                     << setw(9) << setprecision(4) << opt.implied_vol << "  "
                     << setw(9) << setprecision(2) << opt.lambda << "\n";
            }
        }
        cout << "\n";

        cout << "========================================================\n";
        cout << " MODULE 3: Intensity & Hamiltonian Check                \n";
        cout << "========================================================\n";
        
        double alpha = 0.7;
        double beta = 150.0;
        
        // Testing Hamiltonian for the At-The-Money option (K=10)
        IntensityFunction test_intensity(options_spec[2].lambda, alpha, beta, options_spec[2].vega);
        test_intensity.build_lookup_table(-0.05, 0.05, 1000);
        
        cout << "H(0) Check (Must be strictly positive): " << setprecision(6) << test_intensity.H(0.0) << "\n\n";

        cout << "========================================================\n";
        cout << " MODULE 4: 3D HJB PDE Solver                            \n";
        cout << "========================================================\n";
        
        // Prepare the optimal quote lookup tables for all 20 options
        vector<OptionData> options;
        for (const auto& o : options_spec) {
            IntensityFunction intensity(o.lambda, alpha, beta, o.vega);
            // Expanded bounds and high resolution for the PDE solver
            intensity.build_lookup_table(-5.0, 5.0, 2000);
            options.push_back({o.vega, o.z, move(intensity)});
        }
        
        // Construct the flattened 3D grid
        HJBGrid grid;
        grid.n_t = 180;
        grid.n_nu = 30;
        grid.n_Vpi = 40;
        grid.T = 0.0012; 
        grid.nu_min = 0.0144;
        grid.nu_max = 0.0324;
        grid.V_bar = 1e7;
        
        double gamma = 1e-3;
        
        cout << "Grid Dimensions: 180 (time) x 30 (variance) x 40 (vega)\n";
        cout << "Solving HJB equation... (This relies heavily on vector contiguous memory)\n";
        
        auto t_start = chrono::high_resolution_clock::now();
        
        HJBSolver solver(heston, grid, options, gamma);
        solver.solve();
        
        auto t_end = chrono::high_resolution_clock::now();
        double elapsed = chrono::duration<double>(t_end - t_start).count();
        
        // Interpolate to find the peak center of the value function
        double peak_val = solver.v_interp(0, 0.0225, 0.0);
        
        cout << "Solve completed in " << setprecision(3) << elapsed << " seconds.\n";
        cout << "Peak Value Function at t=0, nu=0.0225, Vpi=0: " << setprecision(2) << peak_val << "\n";
        cout << "(Expected magnitude matches paper Figure 2 peak: ~130,000)\n";
        
    } catch (const exception& e) {
        cerr << "Fatal Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}