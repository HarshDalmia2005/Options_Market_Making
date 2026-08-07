#include <iostream>
#include <cmath>
#include <complex>

int main() {
    std::cout << "Hello from test" << std::endl;
    
    // Test complex arithmetic
    std::complex<double> z(1.0, 2.0);
    std::cout << "z = " << z << std::endl;
    std::cout << "sqrt(z) = " << std::sqrt(z) << std::endl;
    std::cout << "exp(z) = " << std::exp(z) << std::endl;
    
    std::cout << "All OK" << std::endl;
    return 0;
}
