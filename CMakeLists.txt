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
find_package(Qt4 4.7.3 REQUIRED QtNetwork)

# Disable QtGui module and setup Qt
set(QT_DONT_USE_QTGUI TRUE)
include(${QT_USE_FILE})
add_definitions(${QT_DEFINITIONS})

# Project include path
include_directories(include)

# Source files
# Headers only need to be added for classes containing the Q_OBJECT macro

# Base files
set(autopin+_HEADERS ${autopin+_HEADERS} include/AutopinPlus/Autopin.h include/AutopinPlus/Watchdog.h include/AutopinPlus/ObservedProcess.h)
set(autopin+_SOURCES ${autopin+_SOURCES} src/AutopinPlus/main.cpp src/AutopinPlus/Autopin.cpp src/AutopinPlus/Watchdog.cpp src/AutopinPlus/Error.cpp src/AutopinPlus/OutputChannel.cpp src/AutopinPlus/AutopinContext.cpp src/AutopinPlus/ObservedProcess.cpp src/AutopinPlus/ProcessTree.cpp src/AutopinPlus/Exception.cpp src/AutopinPlus/Tools.cpp)

# Abstract base classes
set(autopin+_HEADERS ${autopin+_HEADERS} include/AutopinPlus/OSServices.h include/AutopinPlus/ControlStrategy.h include/AutopinPlus/DataLogger.h)
set(autopin+_SOURCES ${autopin+_SOURCES} src/AutopinPlus/Configuration.cpp src/AutopinPlus/PinningHistory.cpp src/AutopinPlus/OSServices.cpp src/AutopinPlus/ControlStrategy.cpp src/AutopinPlus/PerformanceMonitor.cpp src/AutopinPlus/DataLogger.cpp)

# OS independent classes
set(autopin+_SOURCES ${autopin+_SOURCES} src/AutopinPlus/StandardConfiguration.cpp src/AutopinPlus/XMLPinningHistory.cpp)

# Linux-specific classes
add_definitions(-Dos_linux)
set(autopin+_HEADERS ${autopin+_HEADERS} include/AutopinPlus/OS/Linux/OSServicesLinux.h include/AutopinPlus/OS/Linux/TraceThread.h)
set(autopin+_SOURCES ${autopin+_SOURCES} src/AutopinPlus/OS/Linux/OSServicesLinux.cpp  src/AutopinPlus/OS/Linux/TraceThread.cpp)

# Autopin1 control strategy
set(autopin+_HEADERS ${autopin+_HEADERS} include/AutopinPlus/Strategy/Autopin1/Main.h)
set(autopin+_SOURCES ${autopin+_SOURCES} src/AutopinPlus/Strategy/Autopin1/Main.cpp)

# Noop control strategy
set(autopin+_HEADERS ${autopin+_HEADERS} include/AutopinPlus/Strategy/Noop/Main.h)
set(autopin+_SOURCES ${autopin+_SOURCES} src/AutopinPlus/Strategy/Noop/Main.cpp)

# History control strategy
set(autopin+_HEADERS ${autopin+_HEADERS} include/AutopinPlus/Strategy/History/Main.h)
set(autopin+_SOURCES ${autopin+_SOURCES} src/AutopinPlus/Strategy/History/Main.cpp)

# External data logger
set(autopin+_HEADERS ${autopin+_HEADERS} include/AutopinPlus/Logger/External/Main.h)
set(autopin+_SOURCES ${autopin+_SOURCES} src/AutopinPlus/Logger/External/Main.cpp src/AutopinPlus/Logger/External/Process.cpp)

# ClustSafe performance monitor
set(autopin+_SOURCES ${autopin+_SOURCES} src/AutopinPlus/Monitor/ClustSafe/Main.cpp)

# GPerf performance monitor
set(autopin+_SOURCES ${autopin+_SOURCES} src/AutopinPlus/Monitor/GPerf/Main.cpp)

# Perf performance monitor
set(autopin+_SOURCES ${autopin+_SOURCES} src/AutopinPlus/Monitor/Perf/Main.cpp)

# Random performance monitor
set(autopin+_SOURCES ${autopin+_SOURCES} src/AutopinPlus/Monitor/Random/Main.cpp)

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
