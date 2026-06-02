#include "ipc_server.h"

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "dispatcher.h"
#include "ipc_protocol.h"

// ────────────────────────────────────────
//  Platform-specific socket headers
// ────────────────────────────────────────
#if defined(DMP_PLATFORM_LINUX)
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCKET_PATH "/tmp/dmp.sock"
using socket_t = int;
#define INVALID_SOCKET (-1)
#define socket_close ::close

#elif defined(DMP_PLATFORM_WINDOWS)
#include <windows.h>

#define PIPE_PATH L"\\\\.\\pipe\\dmp"
using socket_t = HANDLE;
#define INVALID_SOCKET INVALID_HANDLE_VALUE
#endif

// ────────────────────────────────────────
//  Server state
// ────────────────────────────────────────
static std::atomic<bool> g_running{false};

#if defined(DMP_PLATFORM_LINUX)
static std::atomic<socket_t> g_server_fd{INVALID_SOCKET};
#endif

// ────────────────────────────────────────
//  I/O helpers
// ────────────────────────────────────────

/// Read exactly `n` bytes from `fd` into `buf`.
/// Returns true on success, false on EOF or error.
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
  DWORD total = 0;
  while (total < static_cast<DWORD>(n)) {
    DWORD got = 0;
    BOOL ok = ReadFile(fd, static_cast<uint8_t *>(buf) + total,
                       static_cast<DWORD>(n) - total, &got, nullptr);
    if (!ok || got == 0)
      return false;
    total += got;
  }
  return true;
#endif
}

/// Write exactly `n` bytes from `buf` to `fd`.
/// Returns true on success, false on error.
static bool write_exact(socket_t fd, const void *buf, size_t n) {
#if defined(DMP_PLATFORM_LINUX)
  size_t sent = 0;
  while (sent < n) {
    ssize_t w = ::write(fd, static_cast<const uint8_t *>(buf) + sent, n - sent);
    if (w <= 0)
      return false;
    sent += static_cast<size_t>(w);
  }
  return true;

#elif defined(DMP_PLATFORM_WINDOWS)
  DWORD total = 0;
  while (total < static_cast<DWORD>(n)) {
    DWORD written = 0;
    BOOL ok = WriteFile(fd, static_cast<const uint8_t *>(buf) + total,
                        static_cast<DWORD>(n) - total, &written, nullptr);
    if (!ok || written == 0)
      return false;
    total += written;
  }
  return true;
#endif
}

// ────────────────────────────────────────
//  Client handler
// ────────────────────────────────────────

/// Serve one connected client until the connection is closed or an error
/// occurs. Reads IpcHeader frames in a loop, validates magic, delegates to
/// dispatch_command(), then writes the response back.
static void handle_client(socket_t client) {
  while (g_running.load(std::memory_order_relaxed)) {
    // ── 1. Read header ──────────────────────────────────────
    IpcHeader hdr{};
    if (!read_exact(client, &hdr, sizeof(IpcHeader))) {
      // EOF or broken pipe — client disconnected
      break;
    }

    // ── 2. Validate magic ───────────────────────────────────
    if (hdr.magic != IPC_MAGIC) {
      std::fprintf(stderr,
                   "[ipc_server] bad magic: 0x%04X — closing connection\n",
                   hdr.magic);
      break;
    }

    // ── 3. Guard against oversized body ─────────────────────
    if (hdr.length > MAX_BODY_SIZE) {
      std::fprintf(
          stderr,
          "[ipc_server] body too large: %u bytes — closing connection\n",
          hdr.length);
      break;
    }

    // ── 4. Read body ────────────────────────────────────────
    uint8_t body[MAX_BODY_SIZE];
    if (hdr.length > 0) {
      if (!read_exact(client, body, hdr.length)) {
        std::fprintf(stderr, "[ipc_server] failed to read body (%u bytes)\n",
                     hdr.length);
        break;
      }
    }

    // ── 5. Dispatch → get response buffer ───────────────────
    uint8_t resp_body[MAX_BODY_SIZE]{};
    uint32_t resp_len = 0;

    dispatch_command(hdr.command, body, hdr.length, resp_body, MAX_BODY_SIZE,
                     &resp_len);

    // ── 6. Write response header + body ─────────────────────
    IpcHeader resp_hdr{};
    resp_hdr.magic = IPC_MAGIC;
    resp_hdr.command = hdr.command;
    resp_hdr.reserved = 0x00;
    resp_hdr.length = resp_len;

    if (!write_exact(client, &resp_hdr, sizeof(IpcHeader))) {
      std::fprintf(stderr, "[ipc_server] failed to write response header\n");
      break;
    }

    if (resp_len > 0) {
      if (!write_exact(client, resp_body, resp_len)) {
        std::fprintf(stderr, "[ipc_server] failed to write response body\n");
        break;
      }
    }
  }

  // Close the client socket/pipe handle
#if defined(DMP_PLATFORM_LINUX)
  socket_close(client);
#elif defined(DMP_PLATFORM_WINDOWS)
  FlushFileBuffers(client);
  DisconnectNamedPipe(client);
  CloseHandle(client);
#endif
}

// ────────────────────────────────────────
//  ipc_server_run  (blocking)
// ────────────────────────────────────────
void ipc_server_run() {
  g_running.store(true, std::memory_order_release);

#if defined(DMP_PLATFORM_LINUX)
  // ── Create Unix Domain Socket ────────────────────────────
  socket_t server = ::socket(AF_UNIX, SOCK_STREAM, 0);
  if (server == INVALID_SOCKET) {
    std::perror("[ipc_server] socket");
    return;
  }
  g_server_fd.store(server, std::memory_order_release);

  // Remove stale socket file from a previous run
  ::unlink(SOCKET_PATH);

  sockaddr_un addr{};
  addr.sun_family = AF_UNIX;
  std::strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

  if (::bind(server, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
    std::perror("[ipc_server] bind");
    socket_close(server);
    return;
  }

  if (::listen(server, /*backlog=*/8) < 0) {
    std::perror("[ipc_server] listen");
    socket_close(server);
    ::unlink(SOCKET_PATH);
    return;
  }

  std::printf("[ipc_server] listening on %s\n", SOCKET_PATH);

  // ── Accept loop ──────────────────────────────────────────
  while (g_running.load(std::memory_order_acquire)) {
    socket_t client = ::accept(server, nullptr, nullptr);
    if (client == INVALID_SOCKET) {
      if (!g_running.load(std::memory_order_relaxed)) {
        break; // ipc_server_stop() closed the server fd
      }
      std::perror("[ipc_server] accept");
      continue;
    }

    std::printf("[ipc_server] client connected\n");

    // Single-threaded: serve the client to completion before accepting the
    // next one. Swap for std::thread / thread-pool when concurrent clients
    // are needed.
    handle_client(client);
    std::printf("[ipc_server] client disconnected\n");
  }

  // ── Cleanup ──────────────────────────────────────────────
  socket_close(server);
  ::unlink(SOCKET_PATH);

#elif defined(DMP_PLATFORM_WINDOWS)
  std::printf("[ipc_server] listening on %ls\n", PIPE_PATH);

  // ── Accept loop ──────────────────────────────────────────
  while (g_running.load(std::memory_order_acquire)) {
    // Create a new pipe instance for each client
    HANDLE pipe = CreateNamedPipeW(
        PIPE_PATH, PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
        PIPE_UNLIMITED_INSTANCES,
        /*out_buf=*/static_cast<DWORD>(MAX_BODY_SIZE + sizeof(IpcHeader)),
        /*in_buf=*/static_cast<DWORD>(MAX_BODY_SIZE + sizeof(IpcHeader)),
        /*timeout_ms=*/0, nullptr);

    if (pipe == INVALID_HANDLE_VALUE) {
      std::fprintf(stderr, "[ipc_server] CreateNamedPipe failed: %lu\n",
                   GetLastError());
      break;
    }

    // Block until a client connects (or stop() is called)
    BOOL connected = ConnectNamedPipe(pipe, nullptr);
    if (!connected && GetLastError() != ERROR_PIPE_CONNECTED) {
      CloseHandle(pipe);
      if (!g_running.load(std::memory_order_relaxed))
        break;
      continue;
    }

    std::printf("[ipc_server] client connected\n");
    handle_client(pipe); // owns the handle; closed inside handle_client
    std::printf("[ipc_server] client disconnected\n");
  }
#endif
}

// ────────────────────────────────────────
//  ipc_server_stop  (signal-handler safe)
// ────────────────────────────────────────
void ipc_server_stop() {
  g_running.store(false, std::memory_order_release);

#if defined(DMP_PLATFORM_LINUX)
  // Wake the blocking accept() by closing the server socket.
  // accept() will return -1 with EBADF, the loop checks g_running and exits.
  socket_t fd = g_server_fd.exchange(INVALID_SOCKET, std::memory_order_acq_rel);
  if (fd != INVALID_SOCKET) {
    socket_close(fd);
  }
#elif defined(DMP_PLATFORM_WINDOWS)
  // There is no portable way to interrupt a blocking ConnectNamedPipe without
  // a dummy client connection. Open and immediately close a client handle to
  // unblock the server loop.
  HANDLE dummy = CreateFileW(PIPE_PATH, GENERIC_READ | GENERIC_WRITE, 0,
                             nullptr, OPEN_EXISTING, 0, nullptr);
  if (dummy != INVALID_HANDLE_VALUE)
    CloseHandle(dummy);
#endif
}
