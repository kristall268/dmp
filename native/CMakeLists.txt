cmake_minimum_required(VERSION 3.25)

project(dmp-daemon
  VERSION   0.1.0
  LANGUAGES CXX
)

# ────────────────────────────────────────
#  Language standard
# ────────────────────────────────────────
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
  src/main.cpp
  src/ipc_server.cpp
  src/dispatcher.cpp
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
    src/linux/driver_linux.cpp
    src/linux/device_enum.cpp
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
    /W4
    /WX
    /utf-8
    /permissive-
  )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
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
    WIN32_LEAN_AND_MEAN
    NOMINMAX
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
    setupapi
    cfgmgr32
  )
elseif(DMP_PLATFORM STREQUAL "linux")
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
