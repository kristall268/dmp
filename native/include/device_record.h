#pragma once
#include <cstdint>

enum DriverStatus : uint8_t {
  DRIVER_OK = 0,
  DRIVER_MISSED = 1,
  DRIVER_OUTDATED = 2,
  DRIVER_ERROR = 3,
};

enum DeviceCategory : uint8_t {
  CAT_GPU = 0,
  CAT_NET = 1,
  CAT_USB = 2,
  CAT_AUDIO = 3,
  CAT_STORAGE = 4,
  CAT_OTHERS = 255
};

enum StatusCode : uint8_t {
  STATUS_OK = 0,
  STATUS_ERR_GENERIC = 1,
  STATUS_ERR_NOTFOUND = 2,
  STATUS_ERR_DENIED = 3,
  STATUS_ERR_BUSY = 4
};

struct __attribute__((packed)) DeviceRecord {
  char id[64];             // e.g. "PCI\VEN_10DE" or "/sys/class/net/eth0"
  char name[128];          // e.g. "NVIDIA GeForce RTX 4090"
  char driver_version[32]; // e.g. "560.94" — empty string if missing
  DriverStatus status;
  DeviceCategory category;
  uint8_t padding[2];
};
