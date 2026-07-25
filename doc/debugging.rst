===============
Debugging Tips
===============

Overview
--------

XCSoar offers several ways to debug and explore behaviour without flying:

- **In-app replay and simulator** — built into the main binary (desktop)
- **NMEA/IGC utilities** — ``FeedNMEA``, ``EmulateDevice``, ``Run*`` tools
- **Scripting** — Lua ``init.lua`` and :doc:`lua` replay bindings
- **Native debugger** — :program:`gdb` with XCSoar pretty-printers

Build utilities with ``make debug`` or ``make everything``; see
:ref:`development-workflow` in :doc:`build`. Full utility reference:
:doc:`test_debug_utilities`.

Built-in replay and simulator
-----------------------------

On **Linux and macOS desktop** builds, start XCSoar directly in replay or
simulator mode from the command line::

  ./output/UNIX/bin/xcsoar -replay=flight.igc
  ./output/UNIX/bin/xcsoar -simulator
  ./output/UNIX/bin/xcsoar -fly -profile=test/data/issue-2661/trail-on.prf

``-replay=PATH`` loads an IGC (or other supported log) at startup.
``-simulator`` skips the fly/sim prompt and opens simulator mode (manual
position/speed control — useful for UI testing). ``-profile=FNAME`` loads
a settings profile; ``-datapath=PATH`` selects an alternate data directory
(for example a minimal tree under :file:`test/data/`).

See ``./output/UNIX/bin/xcsoar --help`` for all desktop options.

Replaying NMEA into a running XCSoar
------------------------------------

To feed recorded NMEA into XCSoar over the network (e.g. with Condor or
another simulator sending to a TCP client):

1. In XCSoar device configuration, add a **TCP client** device with IP
   ``127.0.0.1`` and port ``4353``.

2. Build and run ``FeedNMEA`` (see :doc:`test_debug_utilities`)::

     make -j$(nproc) output/UNIX/bin/FeedNMEA
     cat test/data/driver/FLARM/pflaf01.nmea | ./output/UNIX/bin/FeedNMEA tcp 4353

``FeedNMEA`` can also write to a serial port or act as a TCP/UDP server;
see :doc:`test_debug_utilities` for other port modes.

Device and port debugging
-------------------------

- ``EnumeratePorts`` — list serial/TTY devices on the host
- ``EmulateDevice`` — respond to XCSoar as a FLARM, Vega, or ATR833 device
- ``RunDeviceDriver`` — pipe NMEA through a driver parser on stdin
- **socat** — monitor bidirectional serial traffic (Debian package ``socat``)

Details and examples: :doc:`test_debug_utilities` (device utilities and
socat section).

Component testing with Run* utilities
-------------------------------------

Standalone programs exercise one subsystem at a time (task engine, wind
computer, analysis dialog, renderers, …) using ``DebugReplay`` on IGC/NMEA
files. Examples::

  ./output/UNIX/bin/RunTask task.xct test/data/01lz1hq1.igc
  ./output/UNIX/bin/RunAnalysis FLARM test/data/01lz1hq1.igc
  ./output/UNIX/bin/RunDeviceDriver FLARM < test/data/driver/FLARM/pflaf01.nmea

See :doc:`test_debug_utilities` for the full list and patterns.

Lua scripting and automated replay
----------------------------------

Place ``init.lua`` in the XCSoar data directory (``lua/init.lua`` under
``-datapath``) to run scripts at startup. The replay API can bulk-process a
log without stepping through the UI manually — useful for profiling map
draw or backend paths::

  xcsoar.replay.start("test/data/issue-2661/short_2026-06-20_11-47.nmea")
  xcsoar.replay.process_all()

Experiment with scripts using ``RunLua`` (see :doc:`lua`). Replay
bindings: :ref:`lua.replay`.

GNU debugger (gdb)
------------------

XCSoar ships gdb pretty-printers for common types (``GeoPoint``, angles,
etc.). Usage and examples: **Debugging XCSoar** in :doc:`architecture`::

  gdb -ex "source tools/gdb.py" output/UNIX/bin/xcsoar

``Run*`` utilities are small standalone binaries and are often easier to
debug under :program:`gdb` or :program:`valgrind` than the full application.

Redraw counter overlay (e-ink)
------------------------------

To see how often the display actually presents a frame (important on
slow e-ink screens), build with ``DRAW_REDRAW_COUNTER=y``::

  make -j$(nproc) TARGET=UNIX USE_CCACHE=y DRAW_REDRAW_COUNTER=y

The flag is defined in :file:`build/options.mk` (default ``n``). When
enabled, each ``TopWindow::Expose()`` / framebuffer flip draws a
high-contrast overlay in the top-left corner::

  R:<total>  <hz>/s

``R`` is the lifetime present count; the rate is frames over the last
one-second window (it stays ``0.0/s`` until the first full second).
The overlay is paint-only (it does not capture mouse or touch).

On Kobo, or any build where ``HasEPaper()`` is true, use the overlay
while scrolling lists and panels. Paths already gated by ``HasEPaper()``
(for example list pixel pan and kinetic scrolling) should keep the rate
low; spikes mean something is still forcing full-screen presents.
The same make flag works for ``TARGET=KOBO`` and ``OPENGL=n`` builds.

Clean or rebuild affected objects when toggling the flag; mixed objects
with and without ``-DDRAW_REDRAW_COUNTER`` are not reliable.

iOS and macOS
-------------

Use LLDB via Xcode or the VS Code iOS Debug extension; see **Debugging for
iOS and macOS** in :doc:`build`. Run ``make check-ios-sim`` for automated
simulator tests.
