
# Ariadne

Ariadne is a high-performance software router.
Its basic idea is, to feature a clean and expandable programming interface for future developments.
At the moment, it only runs on Linux, but porting it to FreeBSD should be easily possible.

## Architecture

Due to Ariadne's design, the architecture can be adapted for the individual needs of an application.
In its "base form", simply a router is present, using Netmap as its networking framework.
Adding other features commonly found in hardware routers, such as firewall capabilities
or IPsec should be easily feasible.

## Why use Ariadne in the first Place?

### The Production View

In production, Ariadne can speed up the forwarding plane, because it bypasses the
expansive kernel TCP/IP stack, and handles everything it can in user space.

#### Does that mean my other programs won't run anymore?

Other programs relying on network connectivity, such as an SSH or routing daemon
will continue working as normal.
Packets destined to the router itself are re-injected into the kernel stack.
Ariadne is designed to integrate itself into a preexisting ecosystem.

### The Development View

As the Internet is subject to constant change, existing solutions need to be
reevaluated and adapted to fit new requirements.

Ariadne enables developers to rely on a user space based framework, written in
a solid and common language. Due to its design, new modules can be integrated
into the system and pre-existing modules can be withdrawn or extended.

## Compilation

In order to compile Ariadne, some dependencies need to be installed:
* C++ compiler (see below)
* libmnl >= 1.0.3
* cmake
* make
* netmap (headers are sufficient for compilation)
* a POSIX-compliant system
* C++11 ready STL-library

During the course of the development Clang is used as the main compiler.
As long as you stick to "Release" builds GCC >= 4.9 should work without any problem.
"Debug" builds use Clang sanitizers which may not be supported by GCC.
If you are having any problem with either Clang or GCC please open an issue.
Normally such things are easily fixed, since this projects sticks to C++11 without any extension.

In the following we will build the router out-of-tree, in order to keep the
working directory clean. Therefore a new directory is created, cmake
generates the Makefile, which in turn compile the router application.

~~~{.sh}
git submodule init
git submodule update
mkdir build
cd build
cmake ..
make -j4
~~~

## Running Ariadne

~~~{.sh}
./ariadne <interfaces to use>
~~~

## Developing Ariadne

The best way to learn about the internal working of this project is to visit
the Doxygen site:
[https://dschoeffm.github.io/ariadne/](https://dschoeffm.github.io/ariadne/)

You may also want to get familiar with the unit-test setup.
All tests are defined by a single .cpp-file in the `tests` directory.
Every source file is automatically registered to be a test.
As soon as the above build process is finished, all tests can be executed by running `ctest` inside the build directory.
