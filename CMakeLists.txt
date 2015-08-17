# General project settings
project(autopin+)
cmake_minimum_required(VERSION 2.8)

# Enable support for external projects
include(ExternalProject)

########
# Handle different build types
IF(NOT CMAKE_BUILD_TYPE)
   SET(CMAKE_BUILD_TYPE Debug
       CACHE STRING "Choose the type of build : Debug Release."
       FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)
message("* Current build type is : ${CMAKE_BUILD_TYPE}")
string( TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE )
########

########
# Compiler/linker options based on the different compilers
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

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  # Using Clang
  if ("${CMAKE_BUILD_TYPE}" STREQUAL "release")
    ADD_DEFINITIONS(-g -Ofast -march=native)
  endif()

  if ("${CMAKE_BUILD_TYPE}" STREQUAL "debug")
    ADD_DEFINITIONS(-g3 -O0 -Weverything)
  endif()

  # Reduce the noise coming from Qt5
  ADD_DEFINITIONS(-Wno-inconsistent-missing-override -Wno-disabled-macro-expansion -Wno-deprecated)

  # Options required to reduce the noise of spdlog
  ADD_DEFINITIONS(-Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-sign-conversion -Wno-padded -Wno-switch-enum)
  ADD_DEFINITIONS(-Wno-old-style-cast -Wno-undef -Wno-documentation-unknown-command)
  ADD_DEFINITIONS(-Wno-weak-vtables -Wno-global-constructors -Wno-exit-time-destructors -Wno-newline-eof)

elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  # Using GCC
  if ("${CMAKE_BUILD_TYPE}" STREQUAL "release")
    ADD_DEFINITIONS(-g -O2 -march=native)
  endif()

  if ("${CMAKE_BUILD_TYPE}" STREQUAL "debug")
    ADD_DEFINITIONS(-g3 -O0 -Wall -Wextra -Wunused)
  endif()

elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
  # Using Intel C++
  if ("${CMAKE_BUILD_TYPE}" STREQUAL "release")
    ADD_DEFINITIONS(-g -fast)
  endif()

  if ("${CMAKE_BUILD_TYPE}" STREQUAL "debug")
    ADD_DEFINITIONS(-g3 -O0 -Wall -Wextra)
  endif()

endif()

if ("${CMAKE_BUILD_TYPE}" STREQUAL "release")
  # Strip the binary
  set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -s")
endif()
########

########
# Source files
# Base files
set(autopin+_HEADERS ${autopin+_HEADERS} include/AutopinPlus/Autopin.h include/AutopinPlus/Watchdog.h include/AutopinPlus/ObservedProcess.h include/AutopinPlus/AutopinContext.h)
set(autopin+_SOURCES ${autopin+_SOURCES} src/AutopinPlus/main.cpp src/AutopinPlus/Autopin.cpp src/AutopinPlus/Watchdog.cpp src/AutopinPlus/Error.cpp src/AutopinPlus/AutopinContext.cpp src/AutopinPlus/ObservedProcess.cpp src/AutopinPlus/ProcessTree.cpp src/AutopinPlus/Exception.cpp src/AutopinPlus/Tools.cpp)

# Abstract base classes
set(autopin+_HEADERS ${autopin+_HEADERS} include/AutopinPlus/ControlStrategy.h include/AutopinPlus/DataLogger.h)
set(autopin+_SOURCES ${autopin+_SOURCES} src/AutopinPlus/Configuration.cpp src/AutopinPlus/ControlStrategy.cpp src/AutopinPlus/PerformanceMonitor.cpp src/AutopinPlus/DataLogger.cpp)

# OS independent classes
set(autopin+_HEADERS ${autopin+_HEADERS} include/AutopinPlus/MQTTClient.h)
set(autopin+_SOURCES ${autopin+_SOURCES} src/AutopinPlus/StandardConfiguration.cpp src/AutopinPlus/MQTTClient.cpp)

# OS-Service related classes
set(autopin+_HEADERS ${autopin+_HEADERS} include/AutopinPlus/OS/OSServices.h include/AutopinPlus/OS/TraceThread.h include/AutopinPlus/OS/SignalDispatcher.h)
set(autopin+_SOURCES ${autopin+_SOURCES} src/AutopinPlus/OS/OSServices.cpp  src/AutopinPlus/OS/TraceThread.cpp src/AutopinPlus/OS/SignalDispatcher.cpp src/AutopinPlus/OS/CpuInfo.cpp)

# Autopin1 control strategy
set(autopin+_HEADERS ${autopin+_HEADERS} include/AutopinPlus/Strategy/Autopin1/Main.h)
set(autopin+_SOURCES ${autopin+_SOURCES} src/AutopinPlus/Strategy/Autopin1/Main.cpp)

# Noop control strategy
set(autopin+_HEADERS ${autopin+_HEADERS} include/AutopinPlus/Strategy/Noop/Main.h)
set(autopin+_SOURCES ${autopin+_SOURCES} src/AutopinPlus/Strategy/Noop/Main.cpp)

# Compact control strategy
set(autopin+_HEADERS ${autopin+_HEADERS} include/AutopinPlus/Strategy/Compact/Main.h)
set(autopin+_SOURCES ${autopin+_SOURCES} src/AutopinPlus/Strategy/Compact/Main.cpp)

# Scatter control strategy
set(autopin+_HEADERS ${autopin+_HEADERS} include/AutopinPlus/Strategy/Scatter/Main.h)
set(autopin+_SOURCES ${autopin+_SOURCES} src/AutopinPlus/Strategy/Scatter/Main.cpp)

# External data logger
set(autopin+_HEADERS ${autopin+_HEADERS} include/AutopinPlus/Logger/External/Main.h)
set(autopin+_SOURCES ${autopin+_SOURCES} src/AutopinPlus/Logger/External/Main.cpp src/AutopinPlus/Logger/External/Process.cpp)

# ClustSafe performance monitor
set(autopin+_SOURCES ${autopin+_SOURCES} src/AutopinPlus/Monitor/ClustSafe/Main.cpp)

# GPerf performance monitor
set(autopin+_SOURCES ${autopin+_SOURCES} src/AutopinPlus/Monitor/GPerf/Main.cpp)

# Random performance monitor
set(autopin+_SOURCES ${autopin+_SOURCES} src/AutopinPlus/Monitor/Random/Main.cpp)
########

########
# Qt
find_package(Qt5Core REQUIRED)
find_package(Qt5Network REQUIRED)

# Disable QtGui module and setup Qt
set(QT_DONT_USE_QTGUI TRUE)

# Moc the Qt files
QT5_WRAP_CPP(autopin+_HEADERS_MOC ${autopin+_HEADERS})
########



########
# fast-lib
ExternalProject_Add(libfast
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/fast-lib
    CONFIGURE_COMMAND  CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER} cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> ${CMAKE_CURRENT_SOURCE_DIR}/vendor/fast-lib
    BUILD_IN_SOURCE 0
    BUILD_COMMAND ${MAKE})

# Store libmosquitto variable in directory
ExternalProject_Get_Property(libfast install_dir)
set (libfast_path ${install_dir})
########

########
# Includes
# autopin+ include path
include_directories(include)

# spdlog
include_directories(vendor/spdlog/include/spdlog)

# fastlib
include_directories(${libfast_path}/include/fast-lib)
include_directories(${libfast_path}/include/fast-lib/external)


########

########
# Compiling and linking
add_executable(autopin+ ${autopin+_SOURCES} ${autopin+_HEADERS_MOC})
target_link_libraries(autopin+ -lpthread)

# Make sure auotpin+ depends on libfast and link with it
add_dependencies (autopin+ libfast)
target_link_libraries(autopin+ ${libfast_path}/lib/libfastlib_communication.a ${libfast_path}/lib/libfastlib_serialization.a)

# Add Qt5 libraries
qt5_use_modules(autopin+ Core)
qt5_use_modules(autopin+ Network)
########

########
# Add git-cmake-format
option(CLANG_FORMAT "Call clang-format for every call to make" OFF)
if(CLANG_FORMAT)
    set (GCF_IGNORE_LIST "vendor/")
    add_subdirectory(vendor/git-cmake-format)
endif(CLANG_FORMAT)
########

########
# Doxygen
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
########
