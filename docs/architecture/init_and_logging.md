# Init Manager & Logging Architecture

This document explains how the new init manager and kernel logging subsystem work in AltoniumOS. It also describes how to extend the service tree with new subsystems.

## Overview

The legacy boot sequence performed device initialization inline inside `kernel/main.c`. The new init manager (inspired by runit/OpenRC concepts) brings the following improvements:

* deterministic dependency ordering for services like the console, keyboard, disk, filesystem, and shell
* pluggable failure policies so critical services halt the boot while best-effort services can fail without stopping the system
* a single place to inspect service state transitions for future diagnostics

In parallel, the kernel logging subsystem now captures leveled boot events in a ring buffer and persists them to disk once the filesystem is online. The accompanying `logcat` shell command exposes both the in-memory buffer and the persisted `/var/log/boot.log` file for field debugging.

## Init Manager

### Components

* `init/manager.c` – core dependency resolver and service launcher
* `include/init/manager.h` – service descriptor API
* service callbacks defined in `kernel/main.c`

### Service Descriptor Fields

Each service is described via `service_descriptor_t`:

| Field | Purpose |
| --- | --- |
| `name` | Unique identifier used for dependency matching and logging |
| `start` | Callback invoked to start the service |
| `dependencies[]` | NULL-terminated list of named prerequisites |
| `dependency_count` | Number of dependency entries |
| `failure_policy` | What to do when startup fails (`HALT`, `WARN`, `IGNORE`) |
| `status` | Runtime state (stopped, starting, running, failed) |
| `error_code` | Failure reason returned by the callback |

### Boot Flow

1. `kernel_main` calls `init_manager_init` followed by `register_core_services`.
2. Each service declares its dependencies:
   * `console` (none)
   * `keyboard` (console)
   * `bootlog` (console)
   * `disk` (console, bootlog)
   * `filesystem` (disk)
   * `shell` (console, keyboard, filesystem)
3. `init_manager_start_all` loops until every service either runs or fails according to policy. Dependencies must be `RUNNING` before a child is attempted.
4. The manager logs every transition via the new logging macros and mirrors the status on the VGA console for visibility.

### Adding New Services

1. Implement a start callback: `static int service_usb_start(service_descriptor_t *svc) { ... }`
2. Provide dependency names (e.g., `const char *usb_deps[] = { "disk", NULL };`).
3. Register with `init_manager_register_service(&init_manager, "usb", service_usb_start, usb_deps, 1, FAILURE_POLICY_WARN);`
4. Extend documentation/tests as needed.

Services should be quick and re-entrant; long-running daemons belong elsewhere.

## Kernel Logging

### Components

* `kernel/log.c` / `include/kernel/log.h`
* ring buffer with `LOG_BUFFER_SIZE` (4 KiB default)
* leveled macros: `KLOG_DEBUG/INFO/WARN/ERROR`

### Behavior

1. `klog_init` zeros the buffer during early boot.
2. Any subsystem can log messages even before the console is cleared.
3. Once the filesystem service reports `READY`, `klog_set_filesystem_ready(1)` flushes the buffer to `/VAR/LOG/BOOT.LOG`. Missing directories are created automatically (and also seeded in all disk image builders).
4. Subsequent calls to `klog_flush_to_disk` overwrite the log with the latest buffer snapshot.

### Accessing Logs

* `logcat` – defaults to the in-memory buffer, `logcat file` reads `/var/log/boot.log` (requires mounted filesystem).
* `logcat` is useful even when the filesystem failed because it still shows the ring buffer.

## Tests

* `test_init.sh` boots the kernel under QEMU, checks for init-manager output ordering, verifies disk images seed `/VAR/LOG`, and exercises the new command.
* Host-side tests (future extension) can link against `init/manager.c` to validate dependency resolution without booting QEMU.

## Extensibility

* Additional failure policies can be added (e.g., automatic retries) by extending `failure_policy_t` and updating the manager loop.
* The logging subsystem exposes `klog_get_buffer` so future tooling can stream logs over serial ports or network consoles before disks are available.

## Summary

The init manager and logging foundation turn the ad-hoc boot sequence into an extensible, observable pipeline. New services simply register their callbacks and dependencies, while the kernel log preserves the timeline for both on-device inspection and persisted diagnostics.
