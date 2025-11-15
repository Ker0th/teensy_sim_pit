# CMakeList.txt : CMake project for teensy_sim_pit, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.10)

project(teensy_sim_pit LANGUAGES CXX)

# Use C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add source to this project's executable.
add_executable(teensy_sim_pit
  src/teensy_sim_pit.cpp
  hpp/teensy_sim_pit.h
  main.cpp)

target_include_directories(teensy_sim_pit PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/hpp)

# Link Windows SetupAPI for COM device discovery (SetupDi*)
if (WIN32)
  target_link_libraries(teensy_sim_pit PRIVATE setupapi)
endif()

# Use prebuilt iRacing SDK .lib (replace previous source-build)
if (WIN32)
  set(IRS_SDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/SDK/irsdk_1_19")

  # Path to the prebuilt .lib you provided (use forward slashes)
  set(IRS_SDK_PREBUILT_LIB "C:/Code/teensy_sim_pit/SDK/irsdk_1_19/x64/Release/irsdk_client.lib")

  if (NOT EXISTS "${IRS_SDK_PREBUILT_LIB}")
    message(FATAL_ERROR "Prebuilt iRacing SDK library not found: ${IRS_SDK_PREBUILT_LIB}")
  endif()

  # Create an IMPORTED static target that points to the prebuilt .lib
  add_library(irsdk_static STATIC IMPORTED)

  # Support multi-config generators: set config-specific imported locations
  set_target_properties(irsdk_static PROPERTIES
    IMPORTED_LOCATION "${IRS_SDK_PREBUILT_LIB}"
    IMPORTED_LOCATION_RELEASE "${IRS_SDK_PREBUILT_LIB}"
    IMPORTED_LOCATION_DEBUG   "${IRS_SDK_PREBUILT_LIB}"
  )

  # For IMPORTED targets, use INTERFACE for usage requirements so consumers inherit them
  target_include_directories(irsdk_static INTERFACE "${IRS_SDK_ROOT}")
  target_compile_definitions(irsdk_static INTERFACE _CRT_SECURE_NO_WARNINGS)

  # If the SDK requires SetupAPI at link time, expose it so consumers get it automatically
  target_link_libraries(irsdk_static INTERFACE setupapi)

  # Link the SDK into your executable (setupapi will be propagated via the INTERFACE above)
  target_link_libraries(teensy_sim_pit PRIVATE irsdk_static)
endif()