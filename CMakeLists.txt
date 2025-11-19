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
target_include_directories(teensy_sim_pit PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/dependencies)

# Link Windows SetupAPI for COM device discovery (SetupDi*)
if (WIN32)
  target_link_libraries(teensy_sim_pit PRIVATE setupapi)
endif()

# Use prebuilt iRacing SDK .lib (replace previous source-build)
if (WIN32)
  set(IRS_SDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/SDK/irsdk_1_19")
  set(IRS_SDK_PREBUILT_LIB "${IRS_SDK_ROOT}/x64/Release/irsdk_client.lib")

  if (NOT EXISTS "${IRS_SDK_PREBUILT_LIB}")
    message(FATAL_ERROR "Prebuilt iRacing SDK library not found: ${IRS_SDK_PREBUILT_LIB}")
  endif()

  add_library(irsdk_static STATIC IMPORTED)
  set_target_properties(irsdk_static PROPERTIES
    IMPORTED_LOCATION "${IRS_SDK_PREBUILT_LIB}"
    IMPORTED_LOCATION_RELEASE "${IRS_SDK_PREBUILT_LIB}"
    IMPORTED_LOCATION_DEBUG   "${IRS_SDK_PREBUILT_LIB}"
  )
  target_include_directories(irsdk_static INTERFACE "${IRS_SDK_ROOT}")
  target_compile_definitions(irsdk_static INTERFACE _CRT_SECURE_NO_WARNINGS)

  # Try to use a prebuilt miniaudio.lib if present, otherwise compile the single-file implementation into this target.
  set(MINIAUDIO_LIB "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/miniaudio.lib")
  if (EXISTS "${MINIAUDIO_LIB}")
    add_library(miniaudio STATIC IMPORTED)
    set_target_properties(miniaudio PROPERTIES
      IMPORTED_LOCATION "${MINIAUDIO_LIB}"
      IMPORTED_LOCATION_RELEASE "${MINIAUDIO_LIB}"
      IMPORTED_LOCATION_DEBUG   "${MINIAUDIO_LIB}"
    )
    # assume header already in dependencies include dir
  else()
    # compile miniaudio implementation from source (single TU)
    target_sources(teensy_sim_pit PRIVATE src/miniaudio_impl.cpp)
    # no imported target needed
  endif()

  target_link_libraries(teensy_sim_pit PRIVATE irsdk_static)
  if (TARGET miniaudio)
    target_link_libraries(teensy_sim_pit PRIVATE miniaudio)
  endif()
endif()

# Copy entire sounds directory into the target runtime folder after build
add_custom_command(TARGET teensy_sim_pit POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          "${CMAKE_SOURCE_DIR}/sounds"
          "$<TARGET_FILE_DIR:teensy_sim_pit>/sounds"
)