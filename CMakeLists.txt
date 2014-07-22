# General project settings
project(autopin+)
cmake_minimum_required(VERSION 2.6)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  # using Clang
  ADD_DEFINITIONS(-g -Ofast -std=c++11 -Wno-deprecated-register)
#  ADD_DEFINITIONS(-g3 -O0 -std=c++11 -Weverything -Wno-deprecated-register -Wno-c++98-compat -Wno-padded)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  # using GCC
  ADD_DEFINITIONS(-g -O2 -std=c++11 -Wno-deprecated-register)
#  ADD_DEFINITIONS(-g3 -O0 -std=c++11 -Wall -Wextra -Wno-deprecated-register)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
  # using Intel C++
  ADD_DEFINITIONS(-g -fast -std=c++11 -Wno-deprecated-register)
#  ADD_DEFINITIONS(-g3 -O0 -std=c++11 -Wall -Wextra -Wno-deprecated-register)
endif()

# Configuration options
option(OS_LINUX "Build autopin+ for Linux" ON)

# Find Qt 4 installation
find_package(Qt4 4.7.3 REQUIRED QtCore)

# Disable QtGui module and setup Qt
set(QT_DONT_USE_QTGUI TRUE)
include(${QT_USE_FILE})
add_definitions(${QT_DEFINITIONS})

# Project include path
include_directories(src)

# Source files
# Headers only need to be added for classes containing the Q_OBJECT macro

# Base files
set(autopin+_SOURCES ${autopin+_SOURCES} src/main.cpp)
set(autopin+_HEADERS ${autopin+_HEADERS} src/Autopin.h src/ObservedProcess.h)
set(autopin+_SOURCES ${autopin+_SOURCES} src/Autopin.cpp src/Error.cpp src/OutputChannel.cpp src/AutopinContext.cpp src/ObservedProcess.cpp src/ProcessTree.cpp)

# Abstract base classes
set(autopin+_HEADERS ${autopin+_HEADERS} src/OSServices.h src/ControlStrategy.h)
set(autopin+_SOURCES ${autopin+_SOURCES} src/Configuration.cpp src/PinningHistory.cpp src/OSServices.cpp src/ControlStrategy.cpp src/PerformanceMonitor.cpp)

# OS independent
set(autopin+_SOURCES ${autopin+_SOURCES} src/StandardConfiguration.cpp src/XMLPinningHistory.cpp)

# Linux
add_definitions(-Dos_linux)
include_directories(src/os/linux)
set(autopin+_HEADERS ${autopin+_HEADERS} src/os/linux/OSServicesLinux.h src/os/linux/TraceThread.h)
set(autopin+_SOURCES ${autopin+_SOURCES} src/os/linux/OSServicesLinux.cpp  src/os/linux/TraceThread.cpp)

# Control strategies
include_directories(src/strategy/autopin1)
set(autopin+_HEADERS ${autopin+_HEADERS} src/strategy/autopin1/Strategy_autopin1.h)
set(autopin+_SOURCES ${autopin+_SOURCES} src/strategy/autopin1/Strategy_autopin1.cpp)
include_directories(src/strategy/history)
set(autopin+_HEADERS ${autopin+_HEADERS} src/strategy/history/Strategy_history.h)
set(autopin+_SOURCES ${autopin+_SOURCES} src/strategy/history/Strategy_history.cpp)

# Perf
include_directories(src/monitor/perf)
set(autopin+_SOURCES ${autopin+_SOURCES} src/monitor/perf/Monitor_perf.cpp)

# Random performance monitor
include_directories(src/monitor/random)
set(autopin+_SOURCES ${autopin+_SOURCES} src/monitor/random/Monitor_random.cpp)

# Generating the Documentation
find_package(Doxygen)
if (DOXYGEN_FOUND)
  add_custom_command(
     OUTPUT htmldoc
     COMMAND ${DOXYGEN_EXECUTABLE} Doxyfile
     DEPENDS Doxyfile ${autopin+_SOURCES} ${autopin+_HEADERS}
     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )

  add_custom_target(
     doc
     DEPENDS htmldoc
  )

  add_custom_target(
    cleandoc
    COMMAND ${CMAKE_COMMAND} -E remove_directory doc/html
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )
endif (DOXYGEN_FOUND)

# Compiling
QT4_WRAP_CPP(autopin+_HEADERS_MOC ${autopin+_HEADERS})
add_executable(autopin+ ${autopin+_SOURCES} ${autopin+_HEADERS_MOC})
target_link_libraries(autopin+ ${QT_LIBRARIES} ${linklibs} -lpthread)
