# Options Market Making

A C++17 implementation of the paper **"Algorithmic market making for options"** by Bastien Baldacci, Philippe Bergault, and Olivier Guéant (2020).

## Overview

This project tackles the problem of a market maker in charge of a book of options on a single liquid underlying asset. By using a constant-vega approximation, the paper shows that the seemingly high-dimensional stochastic optimal control problem of an option market maker (with $N$ options + spot + variance state variables) can be reduced to just two state variables:
1. Instantaneous variance ($\nu$)
2. Portfolio vega ($V^\pi$)

This dimensionality reduction allows the problem to be solved via a low-dimensional Hamilton-Jacobi-Bellman (HJB) functional equation.

## Modules & Status

The implementation is broken down into 7 distinct modules:

- [x] **Module 1: Heston Model**
  - Defines the physical and risk-neutral dynamics for the spot and variance.
- [x] **Module 2: Option Pricer**
  - Prices European calls under Heston using the Gil-Pelaez characteristic function inversion.
  - Computes exact Vegas via finite difference.
  - Computes Black-Scholes implied volatilities via Newton-Raphson.
  - *Status: Complete. Values precisely match the paper's exact numerical examples.*
- [x] **Module 3: Market Microstructure (Intensity Functions)**
  - Implements logistic fill probabilities and pre-computes the Hamiltonian $H(p)$.
  - *Status: Complete. Confirmed convexity and $H(0) > 0$ requirement.*
- [ ] **Module 4: HJB Solver**
  - The core PDE solver using a monotone explicit Euler scheme on a 3D grid $(t, \nu, V^\pi)$.
- [ ] **Module 5: Optimal Quotes**
  - Extracts the optimal mid-to-bid and ask-to-mid spreads by inverting the intensity function on the solved value function.
- [ ] **Module 6: Simulation Engine**
  - Evaluates the strategy's PnL via Monte-Carlo simulation.
- [ ] **Module 7: Visualization**
  - Python scripts to reproduce all 13 figures from the paper using CSV outputs.

## Mathematical Formulations

### Module 1: Heston Model
The underlying asset $S_t$ and its instantaneous variance $\nu_t$ follow these dynamics under the risk-neutral measure $\mathbb{Q}$ (assuming interest rate $r=0$):
$$ dS_t = \sqrt{\nu_t} S_t dW^S_t $$
$$ d\nu_t = \kappa^\mathbb{Q}(\theta^\mathbb{Q} - \nu_t)dt + \xi \sqrt{\nu_t} dW^\nu_t $$
where the two Wiener processes are correlated: $dW^S_t dW^\nu_t = \rho dt$.

### Module 2: Option Pricer
The price of a European call option is computed using the **Gil-Pelaez Inversion Theorem**:
$$ C = S_0 P_1 - K P_2 $$
The in-the-money probabilities $P_j$ are computed by integrating the complex characteristic function $\phi(u)$:
$$ P_j = \frac{1}{2} + \frac{1}{\pi} \int_0^\infty \text{Re} \left[ \frac{e^{-i u \ln(K)} \phi_j(u)}{i u} \right] du $$
where $\phi_2(u) = \phi(u)$ and $\phi_1(u) = \frac{\phi(u - i)}{S_0}$. 

The option's Vega is calculated with respect to standard deviation, using central finite differences on variance:
$$ \mathcal{V} = \frac{\partial C}{\partial \sqrt{\nu}} = 2\sqrt{\nu} \frac{\partial C}{\partial \nu} $$

### Module 3: Market Microstructure (Intensity Functions)
The market maker's limit orders are executed according to a logistic intensity function depending on the quoted spread $\delta$:
$$ \Lambda(\delta) = \frac{\lambda}{1 + \exp\left(\alpha + \frac{\beta}{\mathcal{V}} \delta\right)} $$
To solve the stochastic optimal control problem, we pre-compute the Hamiltonian $H(p)$ which maximizes the expected rate of revenue:
$$ H(p) = \sup_{\delta} \Lambda(\delta)(\delta - p) $$

## Building the Project

This project uses CMake and requires a compiler that supports **C++17**.

```powershell
# Create build directory
mkdir build
cd build

# Configure and compile
cmake ..
cmake --build .

# Run the executable
.\option_mm.exe
```

> **Note on Windows with MinGW:** If you are using MinGW and your runtime DLLs are not in your PATH, you may need to copy `libstdc++-6-x64.dll` (or equivalent) into the `build` directory for the executable to run.

## Validated Results So Far

The Heston option grid precisely matches the paper's parameters ($S_0 = 10, \nu_0 = 0.0225$):

| K | T | Price (€) | Vega | Implied Vol |
|---|---|-----------|------|-------------|
| 8.0 | 1.0 | 2.06 | 0.41 | 16.24% |
| 9.0 | 1.0 | 1.22 | 0.91 | 15.47% |
| 10.0 | 1.0 | 0.58 | 1.25 | 14.67% |
| 11.0 | 1.0 | 0.22 | 1.05 | 13.99% |
| 12.0 | 1.0 | 0.06 | 0.55 | 13.37% |
