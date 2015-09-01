#
# This file is part of fast-lib.
# Copyright (C) 2015 RWTH Aachen University - ACS
#
# This file is licensed under the GNU Lesser General Public License Version 3
# Version 3, 29 June 2007. For details see 'LICENSE.md' in the root directory.
#

# Default build type: Debug
set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type")
cmake_minimum_required(VERSION 2.8)
project(fast-lib)
if(POLICY CMP0026)
	cmake_policy(SET CMP0026 OLD) # Used to get location of boost libs
endif()
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

# Cache
set(BUILD_SHARED_LIBS OFF CACHE BOOL "If ON build shared, else static libraries.")
if(BUILD_SHARED_LIBS)
	message(FATAL_ERROR "BUILD_SHARED_LIBS=ON is not supported yet. Use static libs instead.")
endif()

# Library names
set(FASTLIB_COMMUNICATION "fastlib_communication")
set(FASTLIB_SERIALIZATION "fastlib_serialization")

# Define version number
set(FASTLIB_VERSION_MAJOR "0")
set(FASTLIB_VERSION_MINOR "1")
set(FASTLIB_VERSION_PATCH "0")
set(FASTLIB_VERSION "${FASTLIB_VERSION_MAJOR}.${FASTLIB_VERSION_MINOR}.${FASTLIB_VERSION_PATCH}")

# Set compiler flags
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" CXX11_SUPPORTED)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" CXX0X_SUPPORTED)
if(CXX11_SUPPORTED)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(CXX0X_SUPPORTED)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
	message(FATAL_ERROR "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support.")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

# Enable support for external projects
include(ExternalProject)

########
# c-ares
ExternalProject_Add(c-ares
	SOURCE_DIR ${PROJECT_SOURCE_DIR}/vendor/c-ares-1.10.0
	CONFIGURE_COMMAND ${PROJECT_SOURCE_DIR}/vendor/c-ares-1.10.0/configure --prefix=<INSTALL_DIR> CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER})

# Store c-ares install directory in variable
ExternalProject_Get_Property(c-ares install_dir)

set(CARES_LIBRARY "${install_dir}/lib/libcares.a")
set(CARES_PATH "${install_dir}")

# Install headers
install(DIRECTORY "${CARES_PATH}/include/" DESTINATION include/fast-lib/external)
########

########
# mosquitto
ExternalProject_Add(mosquitto
	SOURCE_DIR ${PROJECT_SOURCE_DIR}/vendor/mosquitto-1.3.5
	CMAKE_ARGS -DWITH_TLS=OFF -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> -DCMAKE_C_FLAGS=-fpic -DCMAKE_CXX_FLAGS=-fpic -DARES_PATH=${CARES_PATH})

# Make sure mosquitto depends on c-ares
add_dependencies (mosquitto c-ares)

# Store mosquitto variables
ExternalProject_Get_Property(mosquitto install_dir)
set(MOSQUITTO_LIBRARIES ${install_dir}/lib/libmosquittopp.a ${install_dir}/lib/libmosquitto.a ${CARES_LIBRARY}) 
set(MOSQUITTO_INCLUDE_DIR "${install_dir}/include")

# Install headers
install(DIRECTORY "${MOSQUITTO_INCLUDE_DIR}/" DESTINATION include/fast-lib/external)
########

########
# Boost
include(add_boost)
include_directories(SYSTEM ${BoostSourceDir})

# Install headers
install(DIRECTORY "${BoostSourceDir}/boost" DESTINATION include/fast-lib/external)
########

########
# yaml-cpp
ExternalProject_Add(yaml-cpp
	SOURCE_DIR ${PROJECT_SOURCE_DIR}/vendor/yaml-cpp-0.5.1
	CMAKE_ARGS -DCMAKE_CXX_FLAGS=-fpic -DBOOST_ROOT=${BoostSourceDir} -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>)

# Store yaml-cpp variables
ExternalProject_Get_Property(yaml-cpp install_dir)
set(YAMLCPP_LIBRARY "${install_dir}/lib/libyaml-cpp.a")
set(YAMLCPP_INCLUDE_DIR "${install_dir}/include")

# Install headers
install(DIRECTORY "${YAMLCPP_INCLUDE_DIR}/" DESTINATION include/fast-lib/external)
########

# Add support for merging static libraries
include(MergeStaticLibraries)

# Add subdirectories
add_subdirectory(communication)
add_subdirectory(serialization)
enable_testing()
add_subdirectory(examples)
