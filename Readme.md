
Ariadne
========================

Ariadne is a high-performance software router.
Its basic idea is, to feature a clean and expandable programming interface for future developments.

Architecture
------------------------

Due to Ariadne's design, the architecture can be adapted for the individual needs of an application.
In its "base form", simply a router is present, using Netmap as its networking framework.

Compilation
------------------------

In order to compile Ariadne, some dependencies need to be installed:
* C++ compiler (see below)
* libmnl >= 1.0.3
* netmap (header are sufficient for compilation)
* a POSIX-compliant system
* C++11 ready STL-library

During the course of the development GCC as well as Clang are being used.
The following compiler versions are known to work, or not to work:

| Compiler | Working Versions | Not Working Versions |
|:--------:|:----------------:|:--------------------:|
| GCC      | 5.4.0            | <4.9                 |
| Clang    | 3.8.1            |                      |

Do not try to use GCC < 4.9, as it will always fail!

```bash
mkdir build
cd build
cmake ..
make -j4
```

