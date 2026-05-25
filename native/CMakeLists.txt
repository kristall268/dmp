cmake_minimum_required(VERSION 3.25)

project(dmp-daemon
  VERSION   0.1.0
  LANGUAGES C CXX
)

# ────────────────────────────────────────
#  Language standards
# ────────────────────────────────────────
set(CMAKE_C_STANDARD            11)
set(CMAKE_C_STANDARD_REQUIRED   ON)
set(CMAKE_C_EXTENSIONS          OFF)

set(CMAKE_CXX_STANDARD          23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)

# ────────────────────────────────────────
#  Output directories
# ────────────────────────────────────────
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# ────────────────────────────────────────
#  Platform detection
# ────────────────────────────────────────
if(WIN32)
  set(DMP_PLATFORM "windows")
elseif(UNIX AND NOT APPLE)
  set(DMP_PLATFORM "linux")
else()
  message(FATAL_ERROR "Unsupported platform. Only Windows and Linux are supported.")
endif()

message(STATUS "Building for platform: ${DMP_PLATFORM}")

# ────────────────────────────────────────
#  Sources — common (platform-independent)
# ────────────────────────────────────────
set(DMP_COMMON_SOURCES
  src/main.c
  src/ipc_server.c
  src/dispatcher.c
)

# ────────────────────────────────────────
#  Sources — platform-specific
# ────────────────────────────────────────
if(DMP_PLATFORM STREQUAL "windows")
  set(DMP_PLATFORM_SOURCES
    src/windows/driver_win.cpp
    src/windows/device_enum.cpp
  )
elseif(DMP_PLATFORM STREQUAL "linux")
  set(DMP_PLATFORM_SOURCES
    src/linux/driver_linux.c
    src/linux/device_enum.c
  )
endif()

# ────────────────────────────────────────
#  Executable
# ────────────────────────────────────────
add_executable(dmp-daemon
  ${DMP_COMMON_SOURCES}
  ${DMP_PLATFORM_SOURCES}
)

# ────────────────────────────────────────
#  Include paths
# ────────────────────────────────────────
target_include_directories(dmp-daemon PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

# ────────────────────────────────────────
#  Compile options
# ────────────────────────────────────────
if(MSVC)
  target_compile_options(dmp-daemon PRIVATE
    /W4           # High warning level
    /WX           # Warnings as errors
    /utf-8        # Source and execution charset UTF-8
    /permissive-  # Strict conformance
  )
elseif(CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_ID MATCHES "GNU")
  target_compile_options(dmp-daemon PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Werror
    -fno-strict-aliasing
  )
endif()

# ────────────────────────────────────────
#  Platform-specific compile definitions
# ────────────────────────────────────────
if(DMP_PLATFORM STREQUAL "windows")
  target_compile_definitions(dmp-daemon PRIVATE
    DMP_PLATFORM_WINDOWS
    WIN32_LEAN_AND_MEAN   # Exclude rarely-used Windows headers
    NOMINMAX              # Prevent min/max macro pollution
    UNICODE
    _UNICODE
  )
elseif(DMP_PLATFORM STREQUAL "linux")
  target_compile_definitions(dmp-daemon PRIVATE
    DMP_PLATFORM_LINUX
    _GNU_SOURCE
  )
endif()

# ────────────────────────────────────────
#  Platform-specific link libraries
# ────────────────────────────────────────
if(DMP_PLATFORM STREQUAL "windows")
  target_link_libraries(dmp-daemon PRIVATE
    setupapi    # Driver installation
    cfgmgr32    # Device enumeration
  )
elseif(DMP_PLATFORM STREQUAL "linux")
  # libudev via pkg-config
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(UDEV REQUIRED libudev)

  target_include_directories(dmp-daemon PRIVATE ${UDEV_INCLUDE_DIRS})
  target_link_libraries(dmp-daemon PRIVATE ${UDEV_LIBRARIES})
endif()

# ────────────────────────────────────────
#  Install rules
# ────────────────────────────────────────
include(GNUInstallDirs)

install(TARGETS dmp-daemon
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)
