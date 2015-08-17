#
# This file is part of fast-lib.
# Copyright (C) 2015 RWTH Aachen University - ACS
#
# This file is licensed under the GNU Lesser General Public License Version 3
# Version 3, 29 June 2007. For details see 'LICENSE.md' in the root directory.
#

### Define source files.
# Headers
set(HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/serializable.hpp)

# Sources
set(SRC ${CMAKE_CURRENT_SOURCE_DIR}/serializable.cpp 
	${HEADERS})

# Include directories
include_directories(SYSTEM ${YAMLCPP_INCLUDE_DIR})

### Build and installation targets
# Add library
add_library(${FASTLIB_SERIALIZATION} ${SRC})

# Merge with dependent libraries
set(LIBS ${YAMLCPP_LIBRARY})
MERGE_STATIC_LIBRARIES(${FASTLIB_SERIALIZATION} ALL ${LIBS})

# Install
install(TARGETS ${FASTLIB_SERIALIZATION}
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib)
install(FILES ${HEADERS}
	DESTINATION include/fast-lib/serialization)
