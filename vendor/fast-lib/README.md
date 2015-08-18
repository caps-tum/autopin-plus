<!---
This file is part of fast-lib.
Copyright (C) 2015 RWTH Aachen University - ACS

This file is licensed under the GNU Lesser General Public License Version 3
Version 3, 29 June 2007. For details see 'LICENSE.md' in the root directory.
-->

# fast-lib
[![Build Status](https://travis-ci.org/fast-project/fast-lib.svg?branch=master)](https://travis-ci.org/fast-project/fast-lib)

A C++ library for FaST related functionality

### Build instructions
Static libraries can be built with:
```bash
mkdir build && cd build
cmake ..
make
```

When linking the libraries in an executable librt has to be linked in after that (-lrt).
For an example using fast-lib with cmake see fast-project/migration-framework repository.

### Testing
```bash
mosquitto -d 2> /dev/null  
make test  
```
