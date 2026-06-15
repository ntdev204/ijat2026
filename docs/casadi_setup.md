# CasADi Setup and Installation Guide on Jetson Orin Nano (ARM64)

This guide documents the verification and installation instructions for both Python and C++ CasADi on the Jetson Orin Nano platform running Ubuntu 22.04 LTS (ROS 2 Humble).

---

## 🐍 Python CasADi Setup

### 1. Verification
To check if the CasADi Python package is already installed and verify its version:
```bash
python3 -c "import casadi; print(casadi.__version__)"
```

### 2. Installation
On Jetson Orin Nano (ARM64 architecture), pre-built wheels from PyPI might not always be available for the target Python version, or you may need to install it with optimization flag support.

#### Option A: Installing via pip (if pre-built wheels exist)
```bash
pip3 install casadi
```

#### Option B: Building from Source (Recommended for ARM64/Jetson)
If the pip installation fails or lacks optimizations, you can build the Python bindings alongside the C++ libraries:
```bash
git clone https://github.com/casadi/casadi.git -b 3.6.3
cd casadi
mkdir build && cd build
cmake -DWITH_PYTHON=ON -DWITH_COPASI=OFF -DWITH_CLANG=OFF ..
make -j$(nproc)
sudo make install
```
After installation, update your `PYTHONPATH` if required, or ensure it is installed in the global site-packages.

---

## 🛠️ C++ CasADi Setup

To compile and execute the continuous context-adaptive NMPC solver online (if using dynamic C++ generation/linking), the C++ CasADi library is required.

### 1. Verification
Verify that the C++ CasADi headers and shared libraries are installed:
```bash
# Check headers
ls /usr/local/include/casadi/
# Check libraries
ls /usr/local/lib/libcasadi*
```
Or check via `pkg-config` if a pc file was generated:
```bash
pkg-config --modversion casadi
```

### 2. Installation from Source (ARM64)
Since standard APT repositories do not contain CasADi, it must be compiled from source. Follow these instructions:

```bash
# Install dependencies
sudo apt-get update && sudo apt-get install -y \
    build-essential \
    cmake \
    gfortran \
    pkg-config \
    python3-dev \
    python3-numpy \
    liblapack-dev \
    libmetis-dev

# Clone CasADi repository (version 3.6.3 is stable)
git clone https://github.com/casadi/casadi.git -b 3.6.3
cd casadi

# Create build directory
mkdir build && cd build

# Configure build with Python bindings and standard optimizations
cmake -DCMAKE_BUILD_TYPE=Release \
      -DWITH_PYTHON=ON \
      -DWITH_COPASI=OFF \
      -DWITH_CLANG=OFF \
      -DWITH_LAPACK=ON \
      -DWITH_THREAD=ON \
      ..

# Compile using all available CPU cores
make -j$(nproc)

# Install headers, libraries, and Python bindings
sudo make install
sudo ldconfig
```

---

## 🧪 Verification of the Setup

### Python Verification Script
Save the following code as `verify_casadi.py` and run it:
```python
import casadi as ca
import numpy as np

print("CasADi Version:", ca.__version__)

# Define a simple optimization problem
x = ca.MX.sym('x')
obj = (x - 3)**2
nlp = {'x': x, 'f': obj}
solver = ca.nlpsol('solver', 'ipopt', nlp)

# Solve
sol = solver(x0=0)
x_opt = float(sol['x'])
print(f"Optimization check: x_opt = {x_opt:.4f} (Expected: 3.0000)")
assert np.isclose(x_opt, 3.0, atol=1e-4), "CasADi verification failed!"
print("CasADi Python installation is fully functional!")
```

### C++ Verification Program
Create a simple `test_casadi.cpp`:
```cpp
#include <casadi/casadi.hpp>
#include <iostream>

int main() {
    std::cout << "CasADi C++ library version: " << casadi::CasadiMeta::version() << std::endl;
    
    // Create symbolic variables
    casadi::SX x = casadi::SX::sym("x");
    casadi::SX y = casadi::SX::sym("y");
    
    // Define an expression
    casadi::SX f = x * x + y * y;
    
    // Gradient
    casadi::SX grad_f = casadi::SX::gradient(f, x);
    
    std::cout << "Function f: " << f << std::endl;
    std::cout << "Gradient of f w.r.t x: " << grad_f << std::endl;
    
    return 0;
}
```
Compile and run:
```bash
g++ test_casadi.cpp -o test_casadi -lcasadi
./test_casadi
```
Expected output:
```text
CasADi C++ library version: 3.6.3
Function f: (sq(x)+sq(y))
Gradient of f w.r.t x: (2*x)
```
