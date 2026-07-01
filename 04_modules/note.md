# C++20 Modules

C++20 Modules support in CMake is still maturing (CMake 3.28+ / GCC 14+).
This directory contains conceptual examples; the CI builds the other 8 sections.

Key syntax:
```cpp
// math.cppm — module interface unit
export module math;
export int add(int a, int b) { return a + b; }

// main.cpp — module consumer
import math;
int main() { return add(1, 2); }
```

Benefits vs headers:
- No textual inclusion — faster compilation
- No macro leakage across module boundaries
- Explicit export control (export keyword)
- Single compilation per module (no ODR violations)
