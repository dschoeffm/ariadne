# Ariadne Unit Test Setup

Ariadne ships with a couple of unit-tests, which should always succeed.
These tests are not exhaustive, meaning, that they will not catch every bug.

## File Naming Scheme
Every file of the pattern `testXXXX.cpp` defines one single test case.
Each test case is compiled into its own executable and is independent of every other test case.
This doesn't mean, that test cases cannot share code.

Files named like `sampleXXXX.hpp` are generally modules, which are included by test cases.
These modules might for example implement an extra routing table with fixed content, or
a test suite for LPM algorithms.
Samples like these are usually not very large, therefore building a library out of
them and liking it all together is not worth the work. Just include it as a header.
