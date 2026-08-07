#include "heston.hpp"
#include "option_pricer.hpp"
#include <iostream>
#include <iomanip>
#include <vector>

using namespace std;

int main() {
    try {
        cout << "Starting Option Market Maker..." << endl;
        
        // 1. Initialize Heston Parameters (Module 1)
        HestonParams params; 
        cout << "Heston params initialized. S0=" << params.S0 
             << " nu0=" << params.nu0 << endl;
        
        // 2. Initialize Pricer (Module 2)
        HestonPricer pricer(params);
        
        // 3. Build and retrieve the option grid
        cout << "Computing option prices (20 options)..." << endl;
        vector<OptionSpec> grid = pricer.build_option_grid();
        cout << "Done. Computed " << grid.size() << " options.\n" << endl;
        
        // 4. Print results in a formatted table
        cout << fixed << setprecision(4);
        cout << "-------------------------------------------------------------------\n";
        cout << " K     T       Price     Vega      Implied Vol    Base Lambda\n";
        cout << "-------------------------------------------------------------------\n";
        
        for (const auto& opt : grid) {
            if (opt.T == 1.0) {
                cout << setw(5) << setprecision(1) << opt.K << "  "
                     << setw(5) << opt.T << "  "
                     << setw(8) << setprecision(2) << opt.price << "  "
                     << setw(8) << opt.vega << "  "
                     << setw(11) << setprecision(4) << opt.implied_vol << "  "
                     << setw(12) << setprecision(2) << opt.lambda << "\n";
            }
        }
        cout << "-------------------------------------------------------------------\n";
        
    } catch (const exception& e) {
        cout << "Error: " << e.what() << endl;
        return 1;
    }
    
    return 0;
}