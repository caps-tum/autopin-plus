autopin-plus
============

[![Build Status](https://travis-ci.org/autopin/autopin-plus.svg?branch=master)](https://travis-ci.org/autopin/autopin-plus)

Prerequisites
------------

In order to build ```autopin+```, you'll need

 - a C++ compiler supporting C++ 11 (>= g++ 4.8, >= clang 3.3)
 - the **CMake** build system, at least version 2.8
 - the **Qt5** development framework, at least version 5.0.0
 - the userspace development headers from the **Linux** kernel, at least version 3.13

In order to build the optional documentation, you'll also need

 - the **Doxygen** documentation generator
 - the **Graphviz** graph drawing package

Building autopin+
------------

    Currently, only Linux is supported. autopin+ is based on the Qt framework and requires
    CMake for compiling. In the following, it will be assumed that ccmake is used. In order
    to compile autopin+ create the build directory, change to it and execute

      ccmake ..

    Then type "c" to configure the project. After the configuration process the following
    options will be available:

    - CMAKE_BUILD_TYPE: 	Type of the build, should be set to "Release"
    - CMAKE_PREFIX_PATH: 	Path to your Qt distributions CMake files, e.g. ~/Qt5.4.0/5.4/gcc_64/lib/cmake  (if not detected automatically)
    - CMAKE_INSTALL_PREFIX: 	Installation prefix

    When all options are set to the correct values type "c"and then "g" to generate the build
    system. Then execute

      make

    This will start the compilation process. The compiled binary will be placed in the build
    directory.

Creating the documentation
------------

    After configuring the build system with CMake the documentation can be refreshed by
    executing

      make doc

    in the build directory. This only works when Doxygen and graphviz are installed. The
    documentation can be deleted by executing

      make cleandoc

Cleaning the project
------------

    Intermedia build results can be deleted with the command

      make clean

    in the build directory.
