# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What is XCSoar

XCSoar is a tactical glide computer for glider pilots, written in C++20. It runs on Android, iOS, Linux, macOS, Windows, Kobo e-readers, and Raspberry Pi. The core logic is shared; platform-specific code handles rendering, I/O, and sensors.

## Build System

XCSoar uses GNU Make. Build outputs go into `output/<TARGET>/`. The `TARGET` variable selects the platform:

```sh
make TARGET=PC        # Windows (32-bit, default on Windows hosts)
make TARGET=WIN64     # Windows 64-bit
make TARGET=UNIX      # Linux (default on Linux hosts)
make TARGET=ANDROID   # Android APK (requires NDK)
make TARGET=KOBO      # Kobo e-reader
```

Common build flags:
```sh
make TARGET=UNIX DEBUG=n          # release build
make TARGET=UNIX V=2              # verbose (show full compiler commands)
make TARGET=UNIX CLANG=y          # use clang instead of gcc
make TARGET=UNIX USE_CCACHE=y     # enable ccache
```

Build the entire project, then run tests:
```sh
make TARGET=UNIX
make TARGET=UNIX check            # all tests
make TARGET=UNIX testfast         # fast unit tests only
make TARGET=UNIX testslow         # slow integration tests only
```

Run a single test binary after building it:
```sh
make TARGET=UNIX output/UNIX/bin/TestWaypoints
./output/UNIX/bin/TestWaypoints
```

Third-party dependency libraries (zlib, libpng, Boost headers, etc.) are fetched or located via `build/thirdparty.mk`. Boost is header-only and its path is set by the `BOOST` variable (e.g., `BOOST=boost_1_87_0`).

## Code Style

- **C++20** (`Standard: Cpp20` in `.clang-format`)
- 2-space indentation, no tabs, 79-column limit
- CamelCase for class/function names; snake_case for member variables/locals; UPPER_CASE for constants and enums
- Braces on their own line after class/struct/function definitions; braces on same line for control flow
- Format with `clang-format` (config is `.clang-format` at repo root)
- License header: `// SPDX-License-Identifier: GPL-2.0-or-later` + `// Copyright The XCSoar Project`

## Architecture

### Data Flow and Blackboard Pattern

State is shared between threads via a "blackboard" pattern. The two central data structs are:

- **`NMEAInfo`** (`src/NMEA/Info.hpp`) — raw GPS and sensor data received from devices (position, altitude, speed, FLARM traffic, vario, etc.)
- **`DerivedInfo`** (`src/NMEA/Derived.hpp`) — computed flight data (glide ratio, task progress, thermal info, warnings, etc.)

Key blackboard classes:
- **`DeviceBlackboard`** (`src/Blackboard/DeviceBlackboard.hpp`) — holds per-device `NMEAInfo` arrays plus the merged result. Protected by its own `Mutex`.
- **`InterfaceBlackboard`** (`src/Blackboard/InterfaceBlackboard.hpp`) — read-only snapshot used by the UI thread.

### Threading Architecture

Four main threads cooperate via the blackboard:

1. **Device threads** — one per connected device. Parse NMEA or device-specific protocols, write into `DeviceBlackboard::per_device_data[i]`, then call `ScheduleMerge()`.
2. **`MergeThread`** (`src/MergeThread.hpp`) — wakes on `ScheduleMerge()`. Merges per-device data, runs cheap fast calculations (`BasicComputer`, `FlarmComputer`), then triggers `CalculationThread`.
3. **`CalculationThread`** (`src/CalculationThread.hpp`) — runs the full `GlideComputer` pipeline (airspace warnings, task progress, wind, thermals, contest scoring). Writes `DerivedInfo` back to `DeviceBlackboard`.
4. **`DrawThread`** (`src/DrawThread.hpp`) — renders the map asynchronously when a new frame is needed.

### Key Source Directories

| Directory | Purpose |
|---|---|
| `src/Engine/` | Core algorithms with no UI dependencies |
| `src/Engine/Task/` | Task management, ordered/AAT/MAT tasks, Dijkstra path solving, observation zones |
| `src/Engine/Contest/` | OLC/FAI/WeGlide contest scoring (Dijkstra-based solvers) |
| `src/Engine/Airspace/` | Airspace geometry, warning manager |
| `src/Engine/Trace/` | GPS trace storage for snail trail/contest/thermal |
| `src/Computer/` | Converts `NMEAInfo` → `DerivedInfo`: wind, glide ratio, circling detection, thermal locator |
| `src/NMEA/` | `NMEAInfo`, `DerivedInfo`, and related structs |
| `src/Device/Driver/` | Per-device protocol drivers (FLARM, CAI302, LX, LXNAV, Vega, ATR833, …) |
| `src/Device/Port/` | Serial/Bluetooth/USB port abstraction used by drivers |
| `src/MapWindow/` | Moving map rendering (`MapWindow`, `GlueMapWindow`) |
| `src/Renderer/` | Individual map element renderers (waypoints, airspace, task, traffic, …) |
| `src/ui/canvas/` | Platform-abstracted 2D drawing API |
| `src/ui/canvas/gdi/` | Windows GDI backend |
| `src/ui/canvas/memory/` | Software renderer (used by Kobo, framebuffer targets) |
| `src/ui/canvas/egl/` | EGL/OpenGL backend |
| `src/InfoBox/` | InfoBox content computation and rendering |
| `src/Dialogs/` | All UI dialogs |
| `src/thread/` | Threading primitives (wraps pthreads on POSIX, Win32 threads on Windows) |
| `src/co/` | C++20 coroutine utilities for async I/O |
| `src/Blackboard/` | Blackboard classes and listener interfaces |
| `build/` | All GNU Make `.mk` files |
| `test/src/` | Unit and integration test programs |
| `test/data/` | Test fixtures (IGC files, airspace files, waypoint files, NMEA logs) |
| `android/` | Android Java wrapper code |
| `Data/` | Resources: graphics, fonts, language `.po` files, polar data |

### Device Driver Pattern

Each hardware driver lives in `src/Device/Driver/<Name>/` and implements the `Device` interface (`src/Device/Driver.hpp`). Drivers parse incoming data into `NMEAInfo` fields via `ParseNMEA()` and may implement `Declare()` for IGC task declaration and `ReadFlightList()`/`DownloadFlight()` for logger access.

### UI Widget System

The UI is built on `Widget` (`src/Widget/Widget.hpp`) — a lightweight interface with `Prepare()`, `Show()`, `Hide()`, `Save()`. Dialogs compose widgets inside `WidgetDialog`. Forms use `DataField` objects to bind UI controls to typed values.

### Platform Abstraction

Platform-specific code is conditionally compiled based on `TARGET`. The `ui/canvas/` layer abstracts all drawing. Sensor/device access on Android is bridged through JNI (`src/Android/`, `src/java/`). On Apple platforms, `src/Apple/` provides CoreLocation and audio.
