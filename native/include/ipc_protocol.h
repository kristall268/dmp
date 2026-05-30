#pragma once
#include "device_record.h"
#include <cstdint>

#define IPC_MAGIC 0x444D    // 'D' 'M'
#define MAX_BODY_SIZE 65536 // 64 KB

enum IpcCommand : uint8_t {
  CMD_LIST_DEVICES = 0x01,
  CMD_GET_DEVICE = 0x02,
  CMD_INSTALL_DRIVER = 0x10,
  CMD_REMOVE_DRIVER = 0x11,
  CMD_PING = 0xFF,
};

struct __attribute__((packed)) IpcHeader {
  uint16_t magic;
  uint8_t command;
  uint8_t reserved;
  uint32_t length;
};

struct __attribute__((packed)) DeviceIdRequest {
  char id[64];
};

struct __attribute__((packed)) DevicePathRequest {
  char path[256];
};

struct __attribute__((packed)) DeviceListResponse {
  uint32_t count;
  DeviceRecord devices[]; // flexible array: count × 228 bytes
};
struct __attribute__((packed)) StatusResponse {
  StatusCode code;
  uint8_t reserved;
  char message[128]; // human-readable, may be empty
};
