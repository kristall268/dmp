# Architecture

## Overview

Driver Manager Pro is a polyglot project split into two independent processes that communicate over a local socket (IPC). The Java process runs with normal user privileges and handles the GUI and business logic. The native daemon runs with elevated privileges and performs low-level driver operations.

```
┌─────────────────────────────────────────────────────┐
│                  Java Process                        │
│                 (user privileges)                    │
│                                                      │
│   AppLauncher → Main (JavaFX) → HardwareService     │
│                                      ↕               │
│                               IpcClient              │
└──────────────────────────────────────┬──────────────┘
                                       │
                          Local Socket (IPC)
                    Unix Domain Socket / Named Pipe
                                       │
┌──────────────────────────────────────┴──────────────┐
│                 Native Daemon                        │
│              (elevated privileges)                   │
│                                                      │
│   ipc_server ──→ dispatcher ──→ platform layer       │
│                                  ├── Windows         │
│                                  │   SetupAPI        │
│                                  │   CfgMgr32        │
│                                  └── Linux           │
│                                      libudev         │
│                                      /sys/class/     │
└─────────────────────────────────────────────────────┘
```

---

## Module Structure

```
driver-manager-pro/
│
├── src/main/java/org/dmp/       # Java layer
│   ├── AppLauncher.java         # Entry point — launches Main
│   ├── Main.java                # JavaFX Application, root UI
│   ├── HardwareService.java     # Data assembly point, IPC client
│   └── model/
│       └── DeviceSnapshot.java  # Device record (Java record)
│
└── native/                      # Native daemon
    ├── src/
    │   ├── main.c               # Daemon entry point
    │   ├── ipc_server.c         # Local socket server
    │   ├── dispatcher.c         # Routes commands to platform layer
    │   ├── windows/
    │   │   ├── driver_win.cpp   # SetupAPI / CfgMgr32
    │   │   └── device_enum.cpp  # Device enumeration
    │   └── linux/
    │       ├── driver_linux.c   # libudev integration
    │       └── device_enum.c    # /sys/class/ traversal
    ├── include/
    │   ├── ipc_protocol.h       # Shared binary protocol definitions
    │   ├── device_record.h      # DeviceRecord struct
    │   └── platform.h           # Platform abstraction interface
    └── CMakeLists.txt
```

---

## Layers

### Java Layer

| Class | Role |
|-------|------|
| `AppLauncher` | Entry point. Calls `Main.main()` to bypass Fat JAR JavaFX restriction |
| `Main` | JavaFX `Application`. Builds the UI, owns the primary `Stage` |
| `HardwareService` | Sends IPC requests, receives binary responses, assembles `DeviceSnapshot` records |
| `DeviceSnapshot` | Immutable `record` representing a single device with driver status |

### Native Daemon Layer

| Module | Role |
|--------|------|
| `main.c` | Starts the daemon, requests elevated privileges, opens the socket |
| `ipc_server.c` | Accepts connections, reads `IpcHeader`, dispatches to handler |
| `dispatcher.c` | Routes incoming commands to the correct platform implementation |
| `driver_win.cpp` | Windows: installs and removes drivers via SetupAPI |
| `device_enum.cpp` | Windows: enumerates devices via CfgMgr32 |
| `driver_linux.c` | Linux: manages drivers via libudev and modprobe |
| `device_enum.c` | Linux: traverses `/sys/class/` and reads udev attributes |

---

## IPC Binary Protocol

Communication uses a fixed-size header followed by a variable-length body.

### Packet Layout

```
 0       1       2       3       4       5       6       7
┌───────────────┬───────┬───────┬───────────────────────┐
│     magic     │  cmd  │  rsv  │        length         │
│   uint16_t    │ uint8 │ uint8 │       uint32_t        │
└───────────────┴───────┴───────┴───────────────────────┘
│                    body (length bytes)                 │
└────────────────────────────────────────────────────────┘
```

- `magic` — always `0x444D` (`DM`), used to validate the packet
- `cmd` — command identifier (see table below)
- `length` — size of the body in bytes
- Byte order: **Little Endian** on both platforms

### Commands

| Value | Name | Direction | Description |
|-------|------|-----------|-------------|
| `0x01` | `CMD_LIST_DEVICES` | Java → Daemon | Request full device list |
| `0x02` | `CMD_GET_DEVICE` | Java → Daemon | Request single device by id |
| `0x10` | `CMD_INSTALL_DRIVER` | Java → Daemon | Install a driver |
| `0x11` | `CMD_REMOVE_DRIVER` | Java → Daemon | Remove a driver |
| `0xFF` | `CMD_PING` | Java → Daemon | Health check |

### DeviceRecord (228 bytes)

```c
typedef struct __attribute__((packed)) {
    char           id[64];              // device path or hardware id
    char           name[128];           // human-readable device name
    char           driver_version[32];  // e.g. "560.94"
    uint8_t        status;              // DriverStatus enum
    uint8_t        category;            // DeviceCategory enum
    uint8_t        padding[2];          // alignment
} DeviceRecord;                         // total: 228 bytes
```

### DriverStatus

| Value | Name | Meaning |
|-------|------|---------|
| `0` | `DRIVER_OK` | Driver installed and working |
| `1` | `DRIVER_MISSING` | No driver found |
| `2` | `DRIVER_OUTDATED` | Newer version available |
| `3` | `DRIVER_ERROR` | Driver present but faulted |

### DeviceCategory

| Value | Name |
|-------|------|
| `0` | `CAT_GPU` |
| `1` | `CAT_NET` |
| `2` | `CAT_USB` |
| `3` | `CAT_AUDIO` |
| `4` | `CAT_STORAGE` |
| `255` | `CAT_OTHER` |

---

## Data Flow

### List devices (happy path)

```
UI button click
    │
    ▼
HardwareService.requestDeviceList()
    │  writes IpcHeader { magic=0x444D, cmd=0x01, length=0 }
    ▼
Unix Domain Socket / Named Pipe
    │
    ▼
ipc_server.c  reads header, validates magic
    │
    ▼
dispatcher.c  routes CMD_LIST_DEVICES
    │
    ├── Windows: CfgMgr32 → enumerate devices → fill DeviceRecord[]
    └── Linux:   libudev / /sys/class/ → fill DeviceRecord[]
    │
    ▼
ipc_server.c  writes IpcHeader + DeviceListResponse body
    │
    ▼
HardwareService.java  reads ByteBuffer (Little Endian)
    │  parses count × 228-byte DeviceRecord
    ▼
List<DeviceSnapshot>
    │
    ▼
JavaFX TableView  renders rows
```

---

## Platform Abstraction

The native daemon uses a thin C interface to isolate platform-specific code:

```c
// include/platform.h
typedef struct {
    int  (*enumerate_devices)(DeviceRecord* out, uint32_t max_count);
    int  (*install_driver)   (const char* inf_path);
    int  (*remove_driver)    (const char* device_id);
} PlatformOps;

// Resolved at compile time via CMake platform profile
extern PlatformOps g_platform;
```

On Windows `g_platform` is backed by `driver_win.cpp`.  
On Linux `g_platform` is backed by `driver_linux.c`.  
The dispatcher calls only `g_platform.*` — no `#ifdef` in business logic.

---

## Privilege Model

| Action | Process | Privilege |
|--------|---------|-----------|
| Show device list | Java | User |
| Download driver from server | Java | User |
| Parse and display data | Java | User |
| Install driver | Native daemon | Administrator / root |
| Remove driver | Native daemon | Administrator / root |
| Enable / disable device | Native daemon | Administrator / root |

The daemon is started once at application launch with elevated privileges (UAC prompt on Windows, `sudo` on Linux) and keeps the socket open for the lifetime of the session.
