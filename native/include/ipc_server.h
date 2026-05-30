#pragma once

// ────────────────────────────────────────
//  IPC Server — public interface
//
//  ipc_server_run()  — blocking, call from main.cpp
//  ipc_server_stop() — call from signal handler
// ────────────────────────────────────────

void ipc_server_run();
void ipc_server_stop();
