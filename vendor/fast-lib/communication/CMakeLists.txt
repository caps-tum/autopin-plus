#
# This file is part of fast-lib.
# Copyright (C) 2015 RWTH Aachen University - ACS
#
# This file is licensed under the GNU Lesser General Public License Version 3
# Version 3, 29 June 2007. For details see 'LICENSE.md' in the root directory.
#

### Define header, source and lib files.
# Headers
set(HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/communicator.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/mqtt_communicator.hpp)

# Sources
set(SRC ${CMAKE_CURRENT_SOURCE_DIR}/mqtt_communicator.cpp 
	${HEADERS})

# Include directories
include_directories(SYSTEM "${MOSQUITTO_INCLUDE_DIR}")

### Build and installation targets
# Add library
add_library(${FASTLIB_COMMUNICATION} ${SRC})

# Merge with dependent libraries
add_dependencies(${FASTLIB_COMMUNICATION} ${BoostRegexLibs})
get_property(BOOST_REGEX_LIBRARY TARGET ${BoostRegexLibs} PROPERTY LOCATION)
set(LIBS ${MOSQUITTO_LIBRARIES} ${BOOST_REGEX_LIBRARY})
MERGE_STATIC_LIBRARIES(${FASTLIB_COMMUNICATION} ALL "${LIBS}")
target_link_libraries(${FASTLIB_COMMUNICATION} -lrt)

# Install
install(TARGETS ${FASTLIB_COMMUNICATION}
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib)
install(FILES ${HEADERS}
	DESTINATION include/fast-lib/communication)
