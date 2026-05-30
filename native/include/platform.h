#pragma once

#include <cstdint>

#include "device_record.h"

// ────────────────────────────────────────
//  Platform abstraction interface
//
//  The dispatcher calls only g_platform.*
//  No #ifdef in business logic.
// ────────────────────────────────────────
struct PlatformOps {
  // Enumerate all devices on the system.
  // Fills `out` with up to `max_count` records.
  // Returns the number of records written, or -1 on error.
  int (*enumerate_devices)(DeviceRecord *out, uint32_t max_count);

  // Install a driver from the given .inf / archive path.
  // Returns 0 on success, -1 on error.
  int (*install_driver)(const char *path);

  // Remove a driver by device id.
  // Returns 0 on success, -1 on error.
  int (*remove_driver)(const char *device_id);

  // Enable a device by id.
  // Returns 0 on success, -1 on error.
  int (*enable_device)(const char *device_id);

  // Disable a device by id.
  // Returns 0 on success, -1 on error.
  int (*disable_device)(const char *device_id);
};

// Resolved at compile time via CMake platform profile:
//   Windows → driver_win.cpp
//   Linux   → driver_linux.cpp
extern PlatformOps g_platform;
