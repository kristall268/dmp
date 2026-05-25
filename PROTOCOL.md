# IPC Protocol Specification

Binary communication protocol between the Java application and the native daemon over a Local Socket (Unix Domain Socket on Linux, Named Pipe on Windows).

---

## General Rules

- Byte order: **Little Endian** on both platforms
- Strings: **UTF-8**, null-terminated, fixed-length buffers (unused bytes filled with `0x00`)
- Every message starts with a fixed 8-byte `IpcHeader`
- The daemon sends a response for every request
- Magic value `0x444D` (`DM`) must be validated on both sides — invalid magic closes the connection

---

## Header

Every packet — request and response — starts with this 8-byte header.

```
Offset  Size  Type      Field
──────────────────────────────────────────
0       2     uint16_t  magic     (0x444D)
2       1     uint8_t   command
3       1     uint8_t   reserved  (always 0x00)
4       4     uint32_t  length    (body size in bytes)
```

### C definition

```c
// include/ipc_protocol.h

#define IPC_MAGIC 0x444D  // 'D' 'M'

typedef struct __attribute__((packed)) {
    uint16_t magic;
    uint8_t  command;
    uint8_t  reserved;
    uint32_t length;
} IpcHeader;  // 8 bytes
```

### Java reading

```java
ByteBuffer buf = ByteBuffer.wrap(bytes, 0, 8)
                           .order(ByteOrder.LITTLE_ENDIAN);
short  magic   = buf.getShort();  // must equal 0x444D
byte   command = buf.get();
byte   reserved = buf.get();
int    length  = buf.getInt();
```

---

## Commands

| Value  | Constant             | Direction       | Body sent | Body received  |
|--------|----------------------|-----------------|-----------|----------------|
| `0x01` | `CMD_LIST_DEVICES`   | Java → Daemon   | none      | `DeviceListResponse` |
| `0x02` | `CMD_GET_DEVICE`     | Java → Daemon   | `DeviceIdRequest` | `DeviceRecord` |
| `0x10` | `CMD_INSTALL_DRIVER` | Java → Daemon   | `DriverPathRequest` | `StatusResponse` |
| `0x11` | `CMD_REMOVE_DRIVER`  | Java → Daemon   | `DeviceIdRequest` | `StatusResponse` |
| `0xFF` | `CMD_PING`           | Java → Daemon   | none      | `StatusResponse` |

### C constants

```c
#define CMD_LIST_DEVICES    0x01
#define CMD_GET_DEVICE      0x02
#define CMD_INSTALL_DRIVER  0x10
#define CMD_REMOVE_DRIVER   0x11
#define CMD_PING            0xFF
```

---

## Data Types

### DriverStatus

```c
typedef enum : uint8_t {
    DRIVER_OK       = 0,  // installed and working
    DRIVER_MISSING  = 1,  // no driver found
    DRIVER_OUTDATED = 2,  // newer version available
    DRIVER_ERROR    = 3,  // present but faulted
} DriverStatus;
```

### DeviceCategory

```c
typedef enum : uint8_t {
    CAT_GPU     = 0,
    CAT_NET     = 1,
    CAT_USB     = 2,
    CAT_AUDIO   = 3,
    CAT_STORAGE = 4,
    CAT_OTHER   = 255,
} DeviceCategory;
```

### StatusCode (response)

```c
typedef enum : uint8_t {
    STATUS_OK           = 0,
    STATUS_ERR_GENERIC  = 1,
    STATUS_ERR_NOTFOUND = 2,
    STATUS_ERR_DENIED   = 3,  // insufficient privileges
    STATUS_ERR_BUSY     = 4,  // device in use
} StatusCode;
```

---

## Structures

### DeviceRecord (228 bytes)

Represents a single hardware device with its driver state.

```
Offset  Size  Type            Field
────────────────────────────────────────────────
0       64    char[64]        id
64      128   char[128]       name
192     32    char[32]        driver_version
224     1     uint8_t         status   (DriverStatus)
225     1     uint8_t         category (DeviceCategory)
226     2     uint8_t[2]      padding
```

```c
// include/device_record.h

typedef struct __attribute__((packed)) {
    char           id[64];             // e.g. "PCI\VEN_10DE&DEV_2684" or "/sys/class/net/eth0"
    char           name[128];          // e.g. "NVIDIA GeForce RTX 4090"
    char           driver_version[32]; // e.g. "560.94" — empty string if missing
    DriverStatus   status;
    DeviceCategory category;
    uint8_t        padding[2];
} DeviceRecord;  // total: 228 bytes
```

### DeviceListResponse

Variable-length body returned for `CMD_LIST_DEVICES`.

```
Offset  Size               Field
─────────────────────────────────────────────
0       4    uint32_t      count
4       count × 228        DeviceRecord[]
```

```c
typedef struct __attribute__((packed)) {
    uint32_t   count;
    DeviceRecord devices[];  // flexible array member
} DeviceListResponse;
```

### DeviceIdRequest

Request body for `CMD_GET_DEVICE` and `CMD_REMOVE_DRIVER`.

```
Offset  Size  Field
──────────────────────────
0       64    char[64]  id
```

```c
typedef struct __attribute__((packed)) {
    char id[64];
} DeviceIdRequest;  // 64 bytes
```

### DriverPathRequest

Request body for `CMD_INSTALL_DRIVER`.

```
Offset  Size   Field
────────────────────────────────
0       256    char[256]  path    // absolute path to .inf or driver archive
```

```c
typedef struct __attribute__((packed)) {
    char path[256];
} DriverPathRequest;  // 256 bytes
```

### StatusResponse

Response body for `CMD_PING`, `CMD_INSTALL_DRIVER`, `CMD_REMOVE_DRIVER`.

```
Offset  Size  Field
──────────────────────────────────────────────
0       1     uint8_t    code     (StatusCode)
1       1     uint8_t    reserved
2       128   char[128]  message  // human-readable description, may be empty
```

```c
typedef struct __attribute__((packed)) {
    StatusCode code;
    uint8_t    reserved;
    char       message[128];
} StatusResponse;  // 130 bytes
```

---

## Message Exchange Examples

### CMD_PING

```
Java sends:
  IpcHeader { magic=0x444D, cmd=0xFF, reserved=0x00, length=0 }

Daemon responds:
  IpcHeader    { magic=0x444D, cmd=0xFF, reserved=0x00, length=130 }
  StatusResponse { code=STATUS_OK, message="pong" }
```

### CMD_LIST_DEVICES

```
Java sends:
  IpcHeader { magic=0x444D, cmd=0x01, reserved=0x00, length=0 }

Daemon responds:
  IpcHeader         { magic=0x444D, cmd=0x01, reserved=0x00, length=4 + N×228 }
  DeviceListResponse { count=N, devices=[DeviceRecord × N] }
```

### CMD_INSTALL_DRIVER

```
Java sends:
  IpcHeader         { magic=0x444D, cmd=0x10, reserved=0x00, length=256 }
  DriverPathRequest { path="/tmp/nvidia_560.94.inf" }

Daemon responds:
  IpcHeader      { magic=0x444D, cmd=0x10, reserved=0x00, length=130 }
  StatusResponse { code=STATUS_OK, message="Driver installed successfully" }

  — or on failure —

  StatusResponse { code=STATUS_ERR_DENIED, message="Access denied" }
```

---

## Error Handling

- If `magic` does not equal `0x444D` — close the connection immediately, log the error
- If `length` exceeds `MAX_BODY_SIZE` (64 KB) — close the connection, possible malformed packet
- If `command` is unknown — respond with `StatusResponse { code=STATUS_ERR_GENERIC, message="Unknown command" }`
- On daemon crash — Java detects broken pipe on the socket and attempts reconnect with exponential backoff

---

## Socket Path

| Platform | Path |
|----------|------|
| Linux | `/tmp/dmp.sock` (Unix Domain Socket) |
| Windows | `\\.\pipe\dmp` (Named Pipe) |
