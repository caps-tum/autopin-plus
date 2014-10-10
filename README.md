autopin-plus
============

[![Build Status](https://travis-ci.org/autopin/autopin-plus.svg?branch=master)](https://travis-ci.org/autopin/autopin-plus)

Project directories and files
------------

    build		Build directory (initially empty)
    doc			Documentation of the source code
    src			Source code of autopin+

    CMakeLists.txt	CMake configuration file
    Doxyfile		Configuration file for Doxygen
    CHANGELOG		Changelog for autopin+
    CONFIGURATION	User guide for autopin+
    README		This README

Prerequisites
------------

In order to build ```autopin+```, you'll need

 - the **CMake** build system, at least version 2.6
 - the **Qt4** development framework, at least version 4.7.3
 - the userspace development headers from the **Linux** kernel, at least version 3.13

In order to build the optional documentation, you'll also need

 - the **Doxygen** documentation generator
 - the **Graphviz** graph drawing package

Building autopin+
------------

    Currently, only Linux is supported. autopin+ is based on the Qt framework and requires
    CMake for compiling. In the following, it will be assumed that ccmake is used. In order
    to compile autopin+ change to the build directory and execute

      ccmake ..

    Then type "c" to configure the project. After the configuration process the following
    options will be available:

    - CMAKE_BUILD_TYPE: 	Type of the build, should be set to "Release"
    - CMAKE_INSTALL_PREFIX: 	Installation prefix
    - OS_LINUX: 		Compile autopin+ for Linux (has to be "ON")
    - PERFMON_SUPPORT: 		Compile autopin+ with support for perfmon
    - PERF_SUPPORT:		Compile autopin+ with support for perf
    - QT_QMAKE_EXECUTABLE: 	Path to the qmake executable of the Qt distribution which
				will be used for compiling

    The value QT_QMAKE_EXECUTABLE is determined by searching the directories listed in then
    PATH variable of the system. The paths of the Qt distribution which will be used for
    compiling are determined using the qmake binary referenced by this variable. In order to
    use an own Qt distribution (e. g. for static linking) change the value of QT_QMAKE_EXECUTABLE
    to the path of the qmake executable which belongs to this distribution. In order to make sure
    that the paths for this distribution can be determined correctly execute

      qmake -query

    using the qmake binary of the custom Qt distribution. If the paths in the output of the
    command are not correct re-configure the Qt distribution with the correct prefix:

      ./configure ... -prefix /path/to/the/distribution ...

    Otherwise, CMake will not be able to determine the correct paths of the Qt distribution.

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
