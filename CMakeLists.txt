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
ADD_DEFINITIONS(-std=c++11 )
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
  ADD_DEFINITIONS(-Wno-weak-vtables -Wno-global-constructors -Wno-exit-time-destructors)

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

set(PERF_SOURCE "/home" CACHE FILEPATH "Points to the folder that contains the perf tools source")

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
set(autopin+_HEADERS ${autopin+_HEADERS} include/AutopinPlus/OS/OSServices.h include/AutopinPlus/OS/TraceThread.h include/AutopinPlus/OS/SignalDispatcher.h include/AutopinPlus/OS/CpuInfo.h)
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
#PageMigrate performance monitor
set(autopin+_SOURCES ${autopin+_SOURCES} src/AutopinPlus/Monitor/PageMigrate/Main.cpp)
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

#Perf tools 
ExternalProject_Add(perf-numa
    SOURCE_DIR ${PERF_SOURCE}
    CONFIGURE_COMMAND ""
    BUILD_IN_SOURCE 1
    BUILD_COMMAND make DEBUG=1 WERROR=0 NO_LIBPERL=1 NO_LIBPHYTHON=1 NO_DWARF=1 NO_SLANG=1 NO_LIBELF=1 NO_LIBUNWIND=1 NO_BACKTRACE=1  NO_LIBAUDIT=1 NO_LIBBIONIC=1
    INSTALL_COMMAND "" )
    



    
########
# C-ares
ExternalProject_Add(libcares
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/c-ares-1.10.0
    CONFIGURE_COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/vendor/c-ares-1.10.0/configure --prefix=<INSTALL_DIR> CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER}
    BUILD_IN_SOURCE 0
    BUILD_COMMAND ${MAKE})

# Store c-ares install directory in variable
ExternalProject_Get_Property(libcares install_dir)
set (libcares_path ${install_dir})
########

########
# Mosquitto
ExternalProject_Add(libmosquitto
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/mosquitto-1.3.5
    CONFIGURE_COMMAND  CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER} cmake -DWITH_TLS=OFF -DWITH_TLS_PSK=OFF -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> -DARES_PATH=${libcares_path} ${CMAKE_CURRENT_SOURCE_DIR}/vendor/mosquitto-1.3.5
    BUILD_IN_SOURCE 0
    BUILD_COMMAND ${MAKE})

# Make sure mosquitto depends on libcares
add_dependencies (libmosquitto libcares)

# Store libmosquitto variable in directory
ExternalProject_Get_Property(libmosquitto install_dir)
set (libmosquitto_path ${install_dir})
########

########
# Boost
include(vendor/boost/add_boost.cmake)
########

########
# Yaml-Cpp
ExternalProject_Add(libyaml
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/yaml-cpp-0.5.1
    CONFIGURE_COMMAND  CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER} cmake -DBOOST_ROOT=${BoostSourceDir} -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> ${CMAKE_CURRENT_SOURCE_DIR}/vendor/yaml-cpp-0.5.1
    BUILD_IN_SOURCE 0
    BUILD_COMMAND ${MAKE})

#add_dependencies (libyaml ${AllBoostLibs})

# Store libmosquitto variable in directory
ExternalProject_Get_Property(libyaml install_dir)
set (libyaml_path ${install_dir})
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

# libyaml-cpp
include_directories(${libyaml_path}/include)

# Boost
include_directories(${BoostSourceDir})
########

#We need this in order to be able to link perd
include_directories(${PERF_SOURCE}/util/include)
include_directories(${PERF_SOURCE}/arch/x86/include)
include_directories(${PERF_SOURCE}/../include)
include_directories(${PERF_SOURCE}/../../arch/x86/include/uapi)
include_directories(${PERF_SOURCE}/../../arch/x86/include)
include_directories(${PERF_SOURCE}/../../include/uapi)
include_directories(${PERF_SOURCE}/../../include)
include_directories(${PERF_SOURCE}/util)
include_directories(${PERF_SOURCE})
include_directories(${PERF_SOURCE}/../lib)

ADD_DEFINITIONS( -fpermissive  -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -DHAVE_GTK2_SUPPORT -DNO_LIBPERL -DHAVE_TIMERFD_SUPPORT -DNO_DEMANGLE -DHAVE_ON_EXIT_SUPPORT -DHAVE_LIBNUMA_SUPPORT  -DHAVE_ARCH_X86_64_SUPPORT -DHAVE_PERF_REGS_SUPPORT)

########
# Compiling and linking
add_executable(autopin+ ${autopin+_SOURCES} ${autopin+_HEADERS} ${autopin+_HEADERS_MOC})
target_link_libraries(autopin+ -lpthread)

# Make sure auotpin+ depends on libmosquitto and link with it
add_dependencies (autopin+ libmosquitto)
target_link_libraries(autopin+ ${libmosquitto_path}/lib/libmosquitto.a)

# Make sure auotpin+ depends on libcares and link with it
add_dependencies (autopin+ libcares)
target_link_libraries(autopin+ ${libcares_path}/lib/libcares.a)

# Make sure auotpin+ depends on libyaml and link with it
add_dependencies (autopin+ libyaml)
target_link_libraries(autopin+ ${libyaml_path}/lib/libyaml-cpp.a)

# Add Qt5 libraries
qt5_use_modules(autopin+ Core)
qt5_use_modules(autopin+ Network)

#perf libraries
add_dependencies (autopin+ perf-numa)
target_link_libraries(autopin+ ${PERF_SOURCE}/libperf.a)
target_link_libraries(autopin+ ${PERF_SOURCE}/../lib/api/libapikfs.a)
target_link_libraries(autopin+ ${PERF_SOURCE}/../lib/traceevent/libtraceevent.a)
target_link_libraries(autopin+ pthread)
target_link_libraries(autopin+ python2.7)
target_link_libraries(autopin+ dl)
target_link_libraries(autopin+ numa)
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
