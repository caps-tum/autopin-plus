# General project settings
project(autopin+)
cmake_minimum_required(VERSION 2.8)

# Enable support for external projects
include(ExternalProject)

########
# Compiler options based on the different compilers
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  # Using Clang
  ADD_DEFINITIONS(-g -Ofast -march=native -std=c++11 -Wno-deprecated-register)

  #ADD_DEFINITIONS(-g3 -O0 -std=c++11 -Weverything -Wno-c++98-compat -Wno-padded -Wno-deprecated-register)
  # Options required to reduce the noise of spdlog
  #ADD_DEFINITIONS(-Wno-c++98-compat-pedantic -Wno-sign-conversion -Wno-padded -Wno-switch-enum)
  #ADD_DEFINITIONS(-Wno-old-style-cast -Wno-undef -Wno-documentation-unknown-command)
  #ADD_DEFINITIONS(-Wno-weak-vtables -Wno-global-constructors -Wno-exit-time-destructors)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  # Using GCC
  ADD_DEFINITIONS(-g -O2 -std=c++11 -march=native)
  #ADD_DEFINITIONS(-g3 -O0 -std=c++11 -Wall -Wextra -Wunused)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
  # Using Intel C++
  ADD_DEFINITIONS(-g -fast -std=c++11 -Wno-deprecated-register)
  #ADD_DEFINITIONS(-g3 -O0 -std=c++11 -Wall -Wextra -Wno-deprecated-register)
endif()
########

########
# Source files
# Base files
set(autopin+_HEADERS ${autopin+_HEADERS} include/AutopinPlus/Autopin.h include/AutopinPlus/Watchdog.h include/AutopinPlus/ObservedProcess.h include/AutopinPlus/AutopinContext.h)
set(autopin+_SOURCES ${autopin+_SOURCES} src/AutopinPlus/main.cpp src/AutopinPlus/Autopin.cpp src/AutopinPlus/Watchdog.cpp src/AutopinPlus/Error.cpp src/AutopinPlus/AutopinContext.cpp src/AutopinPlus/ObservedProcess.cpp src/AutopinPlus/ProcessTree.cpp src/AutopinPlus/Exception.cpp src/AutopinPlus/Tools.cpp)

# Abstract base classes
set(autopin+_HEADERS ${autopin+_HEADERS} include/AutopinPlus/ControlStrategy.h include/AutopinPlus/DataLogger.h)
set(autopin+_SOURCES ${autopin+_SOURCES} src/AutopinPlus/Configuration.cpp src/AutopinPlus/PinningHistory.cpp src/AutopinPlus/ControlStrategy.cpp src/AutopinPlus/PerformanceMonitor.cpp src/AutopinPlus/DataLogger.cpp)

# OS independent classes
set(autopin+_SOURCES ${autopin+_SOURCES} src/AutopinPlus/StandardConfiguration.cpp src/AutopinPlus/XMLPinningHistory.cpp)

# OS-Service related classes
set(autopin+_HEADERS ${autopin+_HEADERS} include/AutopinPlus/OS/OSServices.h include/AutopinPlus/OS/TraceThread.h include/AutopinPlus/OS/SignalDispatcher.h)
set(autopin+_SOURCES ${autopin+_SOURCES} src/AutopinPlus/OS/OSServices.cpp  src/AutopinPlus/OS/TraceThread.cpp src/AutopinPlus/OS/SignalDispatcher.cpp)

# Autopin1 control strategy
set(autopin+_HEADERS ${autopin+_HEADERS} include/AutopinPlus/Strategy/Autopin1/Main.h)
set(autopin+_SOURCES ${autopin+_SOURCES} src/AutopinPlus/Strategy/Autopin1/Main.cpp)

# Noop control strategy
set(autopin+_HEADERS ${autopin+_HEADERS} include/AutopinPlus/Strategy/Noop/Main.h)
set(autopin+_SOURCES ${autopin+_SOURCES} src/AutopinPlus/Strategy/Noop/Main.cpp)

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
# Qt related stuff
# Find Qt 4 installation
find_package(Qt4 4.7.3 REQUIRED QtCore)
find_package(Qt4 4.7.3 REQUIRED QtNetwork)

# Disable QtGui module and setup Qt
set(QT_DONT_USE_QTGUI TRUE)
include(${QT_USE_FILE})
add_definitions(${QT_DEFINITIONS})

QT4_WRAP_CPP(autopin+_HEADERS_MOC ${autopin+_HEADERS})
########

########
# C-ares related stuff
ExternalProject_Add(libcares
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/c-ares-1.10.0
    CONFIGURE_COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/vendor/c-ares-1.10.0/configure --prefix=<INSTALL_DIR>
    BUILD_IN_SOURCE 0
    BUILD_COMMAND ${MAKE})

# Add c-ares include directory
ExternalProject_Get_Property(libcares install_dir)
set (libcares_path ${install_dir})
########

########
# Mosquitto related stuff
ExternalProject_Add(libmosquitto
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/mosquitto-1.3.5
    CONFIGURE_COMMAND cmake -DWITH_TLS=OFF -DWITH_TLS_PSK=OFF -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> -DARES_PATH=${libcares_path} ${CMAKE_CURRENT_SOURCE_DIR}/vendor/mosquitto-1.3.5
    BUILD_IN_SOURCE 0
    BUILD_COMMAND ${MAKE})

# Make sure mosquitto depends on libcares
add_dependencies (libmosquitto libcares)

# Add mosquitto include directory
ExternalProject_Get_Property(libmosquitto install_dir)
set (libmosquitto_path ${install_dir})
########

########
# Includes
# autopin+ include path
include_directories(include)

# spdlog
include_directories(vendor/spdlog/include/spdlog)

# libcares
include_directories(${libcares_path}/include)

# libmosquitto
include_directories(${libmosquitto_path}/include)
########

########
# Compiling
add_executable(autopin+ ${autopin+_SOURCES} ${autopin+_HEADERS_MOC} ${autopin+_HEADERS})
target_link_libraries(autopin+ ${QT_LIBRARIES} ${linklibs} -lpthread)

# Make sure auotpin+ depends on libcares and link with it
add_dependencies (autopin+ libcares)
target_link_libraries(autopin+ ${libcares_path}/lib/libcares.a)

# Make sure auotpin+ depends on libmosquitto and link with it
add_dependencies (autopin+ libmosquitto)
target_link_libraries(autopin+ ${libmosquitto_path}/lib/libmosquitto.a)
########


########
# Doxygen related stuff
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

