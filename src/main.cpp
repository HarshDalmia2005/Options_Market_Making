#include "intensity.hpp"
#include <iostream>
#include <iomanip>

using namespace std;

int main() {
    try {
        // Values from the paper's parameters for K=10, T=1.0 (Page 8)
        double lambda = 75.6; // 252 * 30 / (1 + 0.7 * |10 - 10|)
        double alpha = 0.7;
        double beta = 150.0;
        double vega = 1.25;

        IntensityFunction intensity(lambda, alpha, beta, vega);
        
        // Build the lookup table between p = -0.05 and p = 0.05
        intensity.build_lookup_table(-0.05, 0.05, 1000);

        cout << fixed << setprecision(6);
        cout << "Testing Hamiltonian Mathematical Properties:\n";
        cout << "------------------------------------------\n";
        
        cout << "1. H(0) > 0 Check:\n";
        cout << "   H(0) = " << intensity.H(0.0) << " (Must be positive)\n\n";

        cout << "2. Convexity and Derivative Check:\n";
        cout << "   p         H(p)        H'(p)\n";
        cout << "---------------------------------\n";
        
        vector<double> p_test = {-0.02, -0.01, 0.0, 0.01, 0.02};
        for (double p : p_test) {
            cout << setw(8) << p << "  " 
                 << setw(10) << intensity.H(p) << "  " 
                 << setw(10) << intensity.H_prime(p) << "\n";
        }
        cout << "---------------------------------\n";
        
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}