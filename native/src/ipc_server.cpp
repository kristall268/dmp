#include "ipc_server.h"

#include <cstdint>
#include <cstdio>
#include <cstring>

#include "ipc_protocol.h"

// Platform Specific Sockets
#if defined(DMP_PLATFORM_LINUX)
#include "sys/socket.h"
#include "sys/un.h"
#include "unistd.h"
#define SOCKET_PATH "/tmp/dmp.sock"
using socket_t = int;
#define INVALID_SOCKET (-1)
#define socket_close ::close

#elif defined(DMP_PLATFORM_WINDOWS)
#include "windows.h"
#define PIPE_PATH "\\\\.\\pipe\\dmp"
using socket_t = HANDLE;
#define INVALID_SOCKET INVALID_HANDLE_VALUE
#endif

// State
static volatile bool g_running = false;

#if defined(DMP_PLATFORM_LINUX)
static socket_t g_server_id = INVALID_SOCKET;
#endif

// ────────────────────────────────────────
//  I/O helpers
// ────────────────────────────────────────
static bool read_exact(socket_t fd, void *buf, size_t n) {
#if defined(DMP_PLATFORM_LINUX)
  size_t got = 0;
  while (got < n) {
    ssize_t r = ::read(fd, static_cast<uint8_t *>(buf) + got, n - got);
    if (r <= 0)
      return false;
    got += static_cast<size_t>(r);
  }
  return true;

#elif defined(DMP_PLATFORM_WINDOWS)
  DWORD got = 0;
  DWORD total = 0;
  while (total < static_cast<DWORD>(n)) {
    BOOL ok = ReadFile(fd, static_cast<uint8_t *>(buf) + total,
                       static_cast<DWORD> n - total, &got, nullptr);
    if (!ok || got == 0)
      return false;
    total += got;
  }
  return true;
#endif
}
