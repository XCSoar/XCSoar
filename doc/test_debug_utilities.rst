Test and Debug Utilities
=========================

The XCSoar test suite includes a collection of standalone utility programs
for debugging, testing, and interactive exploration of XCSoar components
without running the full application. Many of these utilities are prefixed
with ``Run*`` (e.g., ``RunTask``, ``RunAnalysis``, ``RunDeviceDriver``), but
the suite also includes data conversion tools, device emulators, and other
development utilities.

Overview
--------

Run* utilities are standalone programs that exercise specific XCSoar subsystems
in isolation. They serve several purposes:

- **Debugging**: Test individual components with real or simulated data
- **Development**: Interactive tools for exploring component behavior
- **Testing**: Verify functionality with flight data files
- **Device interaction**: Communicate with hardware devices directly

These utilities are built as part of the XCSoar build system and are available
in the output directory after compilation. The exact path depends on your build
target:

- **Unix/Linux**: ``output/UNIX/bin/`` (default, and for flavors like WAYLAND, OPT, FUZZER)
- **Windows**: ``output/PC/bin/`` (default, and for WIN64 flavor)
- **macOS**: ``output/OSX64/bin/`` or ``output/MACOS/bin/`` (default)

**Important**: Many build "targets" are actually flavors that override the base
target. For example, ``TARGET=WAYLAND`` or ``TARGET=OPT`` both build to
``output/UNIX/bin/`` because they override the TARGET to UNIX internally. The
debug utilities are built for the base target (UNIX, PC, etc.), not for the
flavor name.

**Note**: In the examples below, ``output/UNIX/bin/`` is used (typical for Linux
development). Replace ``UNIX`` with your actual base build target if different
(e.g., ``PC`` for Windows, ``OSX64`` for macOS). To find your output directory,
check what was created in the ``output/`` folder after building.

Building Run* Utilities
------------------------

All Run* utilities are automatically built when you compile XCSoar. They are
defined in ``build/test.mk`` and compiled as part of the debug programs target.

To build all Run* utilities:

.. code-block:: bash

   make DEBUG

To build a specific utility:

.. code-block:: bash

   make output/UNIX/bin/RunTask

The utilities are built for the host platform (your development machine), not
for embedded targets like Android or Kobo.

Common Patterns
---------------

Run* utilities follow several common patterns depending on their purpose:

Pattern 1: DebugReplay-Based Utilities
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Many utilities use ``DebugReplay`` to process flight data files (IGC, NMEA).
This pattern is ideal for testing components that need to process historical
flight data.

**Example**: ``RunTask.cpp``, ``RunWindComputer.cpp``, ``RunContestAnalysis.cpp``

**Structure**:

.. code-block:: cpp

   #include "system/Args.hpp"
   #include "DebugReplay.hpp"
   
   int main(int argc, char **argv)
   {
     Args args(argc, argv, "USAGE");
     DebugReplay *replay = CreateDebugReplay(args);
     if (replay == NULL)
       return EXIT_FAILURE;
     
     args.ExpectEnd();
     
     while (replay->Next()) {
       const MoreData &basic = replay->Basic();
       const DerivedInfo &calculated = replay->Calculated();
       
       // Process data here
     }
     
     delete replay;
     return EXIT_SUCCESS;
   }

**Usage**:

.. code-block:: bash

   ./output/UNIX/bin/RunTask task.xct test/data/01lz1hq1.igc

``CreateDebugReplay()`` automatically detects the file format (IGC, NMEA) and
creates an appropriate replay object. It accepts:

- IGC files: ``file.igc`` (auto-detected by extension)
- IGC files with driver prefix: ``IGC file.igc`` (driver name is ignored)
- NMEA files: ``DRIVER file.nmea`` (driver name required for NMEA parsing)

If the file has a ``.igc`` extension, it is always parsed using the native IGC
replay, even if a driver name is specified before it.

Pattern 2: Main.hpp-Based GUI Utilities
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Utilities that need a graphical interface use ``Main.hpp``, which provides
boilerplate for window creation, event loops, and UI initialization.

**Example**: ``RunAnalysis.cpp``, ``RunCanvas.cpp``, ``RunMapWindow.cpp``

**Structure**:

.. code-block:: cpp

   #define ENABLE_LOOK
   #define ENABLE_DIALOG
   #define ENABLE_CMDLINE
   #define ENABLE_PROFILE
   #define USAGE "DRIVER FILE"
   
   #include "Main.hpp"
   
   static void
   Main(UI::Display &display)
   {
     // Create windows, dialogs, etc.
   }

**Available defines**:

- ``ENABLE_LOOK``: Full Look system (fonts, colors, styles)
- ``ENABLE_DIALOG``: Dialog system with DialogLook
- ``ENABLE_CMDLINE``: Command-line argument parsing
- ``ENABLE_PROFILE``: Profile/configuration file support
- ``ENABLE_MAIN_WINDOW``: Main window with event loop
- ``ENABLE_SCREEN``: Screen initialization
- ``ENABLE_BUTTON_LOOK``: Button rendering support

**Usage**:

.. code-block:: bash

   ./output/UNIX/bin/RunAnalysis FLARM test/data/01lz1hq1.igc

Pattern 3: Simple Command-Line Utilities
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Simple utilities that don't need flight data or GUI can use direct command-line
parsing.

**Example**: ``RunDeviceDriver.cpp``, ``RunMD5.cpp``, ``RunSHA256.cpp``

**Structure**:

.. code-block:: cpp

   #include "system/Args.hpp"
   
   int main(int argc, char **argv)
   {
     Args args(argc, argv, "USAGE");
     // Parse arguments
     args.ExpectEnd();
     
     // Process input (stdin, files, etc.)
     return EXIT_SUCCESS;
   }

**Usage**:

.. code-block:: bash

   echo "$GPRMC,..." | ./output/UNIX/bin/RunDeviceDriver FLARM

Pattern 4: Device Interaction Utilities
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Utilities that interact with hardware devices use ``DebugPort`` and device
drivers directly.

**Example**: ``RunFlarmUtils.cpp``, ``RunLX1600Utils.cpp``, ``RunVegaSettings.cpp``

**Structure**:

.. code-block:: cpp

   #include "test/src/DebugPort.hpp"
   #include "Device/Driver.hpp"
   
   int main(int argc, char **argv)
   {
     Args args(argc, argv, "PORT BAUD");
     DebugPort debug_port(args);
     args.ExpectEnd();
     
     ScopeGlobalAsioThread global_asio_thread;
     NullDataHandler handler;
     auto port = debug_port.Open(*asio_thread, *global_cares_channel, handler);
     
     ConsoleOperationEnvironment env;
     if (!port->WaitConnected(env)) {
       return EXIT_FAILURE;
     }
     
     // Interact with device
     return EXIT_SUCCESS;
   }

**Usage**:

.. code-block:: bash

   ./output/UNIX/bin/RunFlarmUtils /dev/ttyUSB0 57600

Using DebugReplay
-----------------

``DebugReplay`` is the primary mechanism for processing flight data in Run*
utilities. It provides:

- **Automatic format detection**: IGC, NMEA, or device-specific formats
- **Incremental processing**: ``Next()`` advances through the flight data
- **Computed data**: ``Basic()`` returns processed ``MoreData``
- **Calculated data**: ``Calculated()`` returns ``DerivedInfo``
- **State management**: Tracks previous states for calculations

**Basic usage**:

.. code-block:: cpp

   DebugReplay *replay = CreateDebugReplay(args);
   if (replay == NULL)
     return EXIT_FAILURE;
   
   while (replay->Next()) {
     const MoreData &basic = replay->Basic();
     const DerivedInfo &calculated = replay->Calculated();
     
     // Access GPS data
     if (basic.location_available) {
       GeoPoint location = basic.location;
       // ...
     }
     
     // Access calculated values
     if (calculated.estimated_wind_available) {
       SpeedVector wind = calculated.estimated_wind;
       // ...
     }
   }
   
   delete replay;

**Available data**:

- ``replay->Basic()``: Current ``MoreData`` (GPS, sensors, etc.)
- ``replay->LastBasic()``: Previous ``MoreData`` (for calculations)
- ``replay->Calculated()``: Current ``DerivedInfo`` (wind, task stats, etc.)
- ``replay->SetCalculated()``: Mutable reference for updating calculations
- ``replay->SetFlyingComputer()``: Access to ``FlyingComputer`` for flight state

**Setting QNH**:

.. code-block:: cpp

   replay->SetQNH(AtmosphericPressure::Standard());
   // Or from user input:
   replay->SetQNH(AtmosphericPressure::HectoPascal(1013.25));

Common Utility Examples
-----------------------

RunTask
~~~~~~~

Tests task calculation with flight data.

**Usage**:

.. code-block:: bash

   ./output/UNIX/bin/RunTask task.xct test/data/01lz1hq1.igc

**What it does**:

- Loads a task file (``.xct``, ``.tsk``, ``.cup``)
- Replays flight data through the task
- Reports task start, finish, and statistics

RunWindComputer
~~~~~~~~~~~~~~~

Tests wind estimation algorithms.

**Usage**:

.. code-block:: bash

   ./output/UNIX/bin/RunWindComputer FLARM test/data/01lz1hq1.igc

**What it does**:

- Processes flight data through wind computer
- Outputs wind estimates over time
- Useful for debugging wind calculation algorithms

RunDeviceDriver
~~~~~~~~~~~~~~~

Tests device driver NMEA parsing.

**Usage**:

.. code-block:: bash

   echo "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A" | \
     ./output/UNIX/bin/RunDeviceDriver FLARM

**What it does**:

- Parses NMEA sentences through device driver
- Outputs parsed data fields
- Useful for testing driver parsing logic

RunAnalysis
~~~~~~~~~~~

Opens the analysis dialog with flight data.

**Usage**:

.. code-block:: bash

   ./output/UNIX/bin/RunAnalysis FLARM test/data/01lz1hq1.igc

**What it does**:

- Loads flight data
- Opens the analysis dialog window
- Allows interactive exploration of flight data

RunFlarmUtils
~~~~~~~~~~~~~

Interactive tool for configuring FLARM devices.

**Usage**:

.. code-block:: bash

   ./output/UNIX/bin/RunFlarmUtils /dev/ttyUSB0 57600

**What it does**:

- Connects to FLARM device
- Provides interactive menu for:
  - Changing pilot/co-pilot names
  - Setting competition ID/class
  - Configuring stealth mode
  - Adjusting range and baud rate

FeedNMEA
~~~~~~~~

Feeds NMEA sentences from stdin to a serial port, TCP connection, or UDP
socket. Useful for testing XCSoar with simulated GPS data, especially when
running XCSoar under WINE or testing network-based device connections.

**Usage**:

Serial port:

.. code-block:: bash

   cat test.nmea | ./output/UNIX/bin/FeedNMEA /dev/ttyUSB0 57600
   # Or with a pseudo-TTY for WINE:
   cat test.nmea | ./output/UNIX/bin/FeedNMEA /tmp/nmea 57600

TCP server (listener):

.. code-block:: bash

   cat test.nmea | ./output/UNIX/bin/FeedNMEA tcp 4353

TCP client (connects to server):

.. code-block:: bash

   cat test.nmea | ./output/UNIX/bin/FeedNMEA tcp_client 192.168.1.100 4353

UDP listener:

.. code-block:: bash

   cat test.nmea | ./output/UNIX/bin/FeedNMEA udp 4353

**What it does**:

- Reads NMEA sentences from stdin (one per line)
- Writes them to the specified port (serial, TCP, or UDP)
- Automatically sleeps for 1 second when timestamps change (for realistic timing)
- Echoes received data to stdout (for bidirectional communication testing)

**Port Types**:

- **Serial ports**: ``/dev/ttyUSB0``, ``/dev/ttyACM0``, ``COM1`` (Windows), etc.
- **TCP listener**: ``tcp <port>`` - Listens for incoming TCP connections
- **TCP client**: ``tcp_client <ip_address> <port>`` - Connects to a TCP server
- **UDP listener**: ``udp <port>`` - Listens for UDP datagrams
- **Pseudo-TTY**: ``pty <path>`` - Creates a pseudo-terminal

**TCP Server Mode**:

TCP listener mode allows XCSoar (or other clients) to connect and receive NMEA
data over the network:

.. code-block:: bash

   # Terminal 1: Start FeedNMEA as TCP server
   cat flight.nmea | ./output/UNIX/bin/FeedNMEA tcp 4353
   
   # Terminal 2: XCSoar connects to localhost:4353 to receive NMEA data
   # Configure XCSoar device port as "TCP Client" with host "localhost" and port 4353

**TCP Client Mode**:

TCP client mode connects to a remote server and sends NMEA data:

.. code-block:: bash

   # Connect to remote NMEA server and feed data
   cat flight.nmea | ./output/UNIX/bin/FeedNMEA tcp_client 192.168.1.100 4353

This is useful for:
- Sending NMEA data to a remote XCSoar instance
- Testing network-based device connections
- Integrating with network GPS servers

**WINE Integration**:

FeedNMEA is particularly useful for testing XCSoar under WINE:

1. Create a pseudo-TTY: ``./output/UNIX/bin/FeedNMEA /tmp/nmea 57600``
2. Symlink WINE COM port: ``ln -s /tmp/nmea ~/.wine/dosdevices/com1``
3. Configure XCSoar to use COM1
4. Feed NMEA data: ``cat flight.nmea | ./output/UNIX/bin/FeedNMEA /tmp/nmea 57600``

**Example**:

.. code-block:: bash

   # Convert IGC to NMEA and feed to serial port
   ./output/UNIX/bin/IGC2NMEA test/data/01lz1hq1.igc /tmp/flight.nmea
   cat /tmp/flight.nmea | ./output/UNIX/bin/FeedNMEA /dev/ttyUSB0 57600
   
   # Or feed to TCP server for network testing
   cat /tmp/flight.nmea | ./output/UNIX/bin/FeedNMEA tcp 4353

EmulateDevice
~~~~~~~~~~~~~

Emulates device protocols to test XCSoar's device communication without
actual hardware. Supports Vega, FLARM, and ATR833 devices.

**Usage**:

.. code-block:: bash

   ./output/UNIX/bin/EmulateDevice FLARM /dev/ttyUSB0 57600
   ./output/UNIX/bin/EmulateDevice Vega /tmp/nmea 57600
   ./output/UNIX/bin/EmulateDevice ATR833 /dev/ttyUSB0 9600

**What it does**:

- Creates a device emulator that responds to XCSoar commands
- Implements device-specific protocols:
  - **FLARM**: Responds to configuration commands (PFLAC), sends traffic data
  - **Vega**: Emulates variometer with settings commands
  - **ATR833**: Emulates radio transponder with frequency commands
- Runs until the port connection fails
- Useful for testing device drivers without physical hardware

**Supported Devices**:

- ``FLARM``: FLARM collision avoidance system
- ``Vega``: Vega variometer
- ``ATR833``: ATR833 radio transponder

**Use Cases**:

- Testing device driver implementations
- Debugging device communication protocols
- Developing new device drivers
- Automated testing of device interactions

**Example Workflow**:

.. code-block:: bash

   # Terminal 1: Start device emulator
   ./output/UNIX/bin/EmulateDevice FLARM /tmp/flarm 57600
   
   # Terminal 2: Connect XCSoar to /tmp/flarm and test FLARM features
   # XCSoar will send commands, emulator will respond appropriately

Data Conversion Utilities
--------------------------

IGC2NMEA
~~~~~~~~

Converts IGC flight files to NMEA format. Useful for testing device drivers
or feeding NMEA data to other tools.

**Usage**:

.. code-block:: bash

   ./output/UNIX/bin/IGC2NMEA test/data/01lz1hq1.igc output.nmea

**What it does**:

- Reads IGC file using DebugReplay
- Generates NMEA sentences (GPRMC, GPGGA, PGRMZ)
- Writes NMEA output to file
- Useful for converting flight data to NMEA format for testing

**Example**:

.. code-block:: bash

   # Convert IGC to NMEA, then feed to device
   ./output/UNIX/bin/IGC2NMEA flight.igc flight.nmea
   cat flight.nmea | ./output/UNIX/bin/FeedNMEA /dev/ttyUSB0 57600

lxn2igc
~~~~~~~

Converts LX Navigation (LXN) binary files to IGC format.

**Usage**:

.. code-block:: bash

   ./output/UNIX/bin/lxn2igc flight.lxn > flight.igc

**What it does**:

- Reads LX Navigation binary format (``.lxn`` files)
- Converts to standard IGC format
- Outputs to stdout
- Useful for converting old LX device recordings

**Example**:

.. code-block:: bash

   ./output/UNIX/bin/lxn2igc test/data/lxn_to_igc/18BF14K1.FIL > converted.igc

Flight Analysis Utilities
-------------------------

RunContestAnalysis
~~~~~~~~~~~~~~~~~~

Analyzes flight data through all contest solvers and prints results.

**Usage**:

.. code-block:: bash

   # IGC file (auto-detected by extension):
   ./output/UNIX/bin/RunContestAnalysis flight.igc

   # NMEA file (driver name required):
   ./output/UNIX/bin/RunContestAnalysis FLARM flight.nmea

**What it does**:

- Processes flight data through all contest solvers
- Runs exhaustive solving for OLC Classic, FAI, Sprint, League, Plus,
  DMSt (quadrilateral, triangle, out-and-return, free), XContest,
  SIS-AT, WeGlide (distance, FAI, OR, free), and Charron
- Prints distance, score, speed, and time for each contest type
- Useful for comparing contest results across rule sets

**Example output** (abbreviated):

.. code-block:: text

   classic
   #   score 307.294
   #   distance 307.294 (km)
   #   speed 67.213 (kph)
   #   time 16459 (sec)
   dmst
   # quadrilateral
   #   score 276.402
   #   distance 276.402 (km)
   ...
   # triangle
   #   score 276.853
   #   distance 197.752 (km)
   ...

AnalyseFlight
~~~~~~~~~~~~~

Analyzes flight data and outputs JSON with flight phases, takeoff/landing
times, and contest results.

**Usage**:

.. code-block:: bash

   # IGC file (auto-detected by extension):
   ./output/UNIX/bin/AnalyseFlight flight.igc

   # NMEA file (driver name required):
   ./output/UNIX/bin/AnalyseFlight FLARM flight.nmea

   # With options:
   ./output/UNIX/bin/AnalyseFlight --full-points=1024 flight.igc

**What it does**:

- Processes flight data through flight phase detector
- Detects takeoff, release, and landing events
- Calculates contest results (OLC Plus, DMSt)
- Outputs JSON with flight statistics
- Useful for automated flight analysis

**Output includes**:

- Takeoff/release/landing times and locations
- Flight phases (circling, cruise, etc.)
- Contest scores (OLC Plus: classic/triangle/plus; DMSt: quadrilateral/triangle/out-and-return/free)
- Circling statistics

**Note**: Available on UNIX/PC platforms only (not Android).

Device and Port Utilities
--------------------------

EnumeratePorts
~~~~~~~~~~~~~~

Lists all available serial/TTY ports on the system. Useful for debugging
device connection issues.

**Usage**:

.. code-block:: bash

   ./output/UNIX/bin/EnumeratePorts

**What it does**:

- Scans system for available TTY/serial ports
- Lists port paths (e.g., ``/dev/ttyUSB0``, ``/dev/ttyACM0``)
- Helps identify which port a device is connected to

**Example output**:

.. code-block:: bash

   /dev/ttyUSB0
   /dev/ttyUSB1
   /dev/ttyACM0

**Note**: Only available on POSIX systems (Linux, macOS, not Windows).

Monitoring Device Communication with socat
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

`socat` is a powerful command-line utility available in Debian/Ubuntu that can
monitor bidirectional serial port traffic between XCSoar and devices. This is
useful for debugging device communication protocols and seeing all NMEA
sentences in both directions.

**Installation**:

.. code-block:: bash

   sudo apt install socat

**Basic Usage - Monitor Serial Port**:

To monitor a serial port and see all traffic in both directions:

.. code-block:: bash

   # Monitor /dev/ttyUSB0 at 57600 baud, showing both TX and RX
   socat -x -v /dev/ttyUSB0,raw,echo=0,b57600 -

The flags:
- ``-x``: Shows hex dump of all data
- ``-v``: Verbose output with direction indicators (``>`` for TX, ``<`` for
  RX)
- ``-``: Outputs to stdout

**Spying on XCSoar Device Communication**:

To spy on communication between XCSoar and a device, create a virtual serial
pair and route traffic through it:

.. code-block:: bash

   # Terminal 1: Create virtual serial pair with logging
   socat -x -v pty,raw,echo=0,link=/dev/ttyV0 pty,raw,echo=0,link=/dev/ttyV1

   # Terminal 2: Connect device to one end (e.g., /dev/ttyV0)
   # Terminal 3: Configure XCSoar to use /dev/ttyV1
   # All traffic will be logged in Terminal 1

**Alternative: Direct Port Monitoring**:

If you can temporarily disconnect the device, monitor it directly:

.. code-block:: bash

   # Connect device to /dev/ttyUSB0, monitor at 57600 baud
   socat -x -v /dev/ttyUSB0,raw,echo=0,b57600 - | tee device_traffic.log

This will:
- Display all traffic in real-time with hex dumps
- Show direction indicators (``>`` for data sent to device, ``<`` for data
  received from device)
- Save output to ``device_traffic.log`` file

**What it does**:

- Monitors bidirectional serial port traffic
- Shows both TX (transmitted) and RX (received) data
- Displays data in both ASCII and hex format
- Useful for debugging device communication issues
- Helps understand device protocol behavior

**Note**: Requires Debian/Ubuntu package ``socat``. The device port must be
available (not in use by XCSoar) when monitoring directly.

CAI302Tool
~~~~~~~~~~

Interactive tool for CAI302 flight computer devices. Allows reading device
information, waypoints, and flight data.

**Usage**:

.. code-block:: bash

   ./output/UNIX/bin/CAI302Tool /dev/ttyUSB0 9600

**What it does**:

- Connects to CAI302 device
- Reads device information (ID, firmware version)
- Can read waypoints and flight data
- Useful for debugging CAI302 device communication

**Note**: Requires CAI302 device connected via serial port.

Scripting Utilities
--------------------

RunLua
~~~~~~

Runs Lua scripts with XCSoar Lua bindings. Useful for testing Lua scripts
or developing Lua-based features.

**Usage**:

.. code-block:: bash

   ./output/UNIX/bin/RunLua script.lua

**What it does**:

- Initializes Lua state with XCSoar bindings
- Loads and executes Lua script
- Provides access to:
  - Geo functions (coordinate calculations)
  - HTTP functions (if HTTP support enabled)
  - Log functions
  - Custom ``alert()`` function for output

**Available bindings**:

- ``geo.*``: Geographic calculations
- ``http.*``: HTTP requests (if enabled)
- ``log.*``: Logging functions
- ``alert()``: Print to stderr

**Example script**:

.. code-block:: lua

   local point1 = geo.Point(52.0, 13.0)
   local point2 = geo.Point(52.1, 13.1)
   local distance = geo.Distance(point1, point2)
   alert("Distance: " .. distance)

**Note**: Requires Lua support to be enabled in build (``LUA=y``).

Renderer Utilities
-------------------

These utilities test and demonstrate various rendering components used in XCSoar's
UI. They provide interactive visual testing of renderers.

RunRenderOZ
~~~~~~~~~~~

Tests observation zone (OZ) rendering for different task zone types.

**Usage**:

.. code-block:: bash

   ./output/UNIX/bin/RunRenderOZ

**What it does**:

- Displays a window with a list of observation zone types on the left
- Renders the selected zone type on the right
- Supports all OZ types: Line, Cylinder, Sector, FAI Sector, Keyhole, etc.
- Interactive: click on zone type to see it rendered
- Useful for testing OZ rendering and debugging zone shapes

**Supported zone types**:

- Line, Cylinder, MAT Cylinder
- Sector, FAI Sector, Annular Sector
- DAeC Keyhole, BGA Fixed Course, BGA Enhanced Option
- BGA Start, Symmetric Quadrant, Custom Keyhole

RunChartRenderer
~~~~~~~~~~~~~~~~

Tests chart/graph rendering capabilities.

**Usage**:

.. code-block:: bash

   ./output/UNIX/bin/RunChartRenderer

**What it does**:

- Displays a window with chart types on the left
- Renders the selected chart on the right
- Demonstrates line charts, grids, labels
- Interactive: select chart type from list
- Useful for testing chart rendering and data visualization

RunWindArrowRenderer
~~~~~~~~~~~~~~~~~~~~

Tests wind arrow rendering with animated wind display.

**Usage**:

.. code-block:: bash

   ./output/UNIX/bin/RunWindArrowRenderer

**What it does**:

- Displays animated wind arrow in center of window
- Wind direction and speed change automatically
- Demonstrates wind arrow rendering styles
- Useful for testing wind visualization

RunHorizonRenderer
~~~~~~~~~~~~~~~~~~

Tests artificial horizon rendering with animated attitude display.

**Usage**:

.. code-block:: bash

   ./output/UNIX/bin/RunHorizonRenderer

**What it does**:

- Displays animated artificial horizon
- Bank and pitch angles change automatically
- Demonstrates horizon rendering with attitude indicators
- Useful for testing attitude visualization

RunFinalGlideBarRenderer
~~~~~~~~~~~~~~~~~~~~~~~~

Tests final glide bar rendering with animated altitude difference.

**Usage**:

.. code-block:: bash

   ./output/UNIX/bin/RunFinalGlideBarRenderer

**What it does**:

- Displays final glide bar with altitude difference
- Shows MC and MC0 solutions
- Animated: altitude difference changes over time
- Useful for testing final glide calculations and rendering

RunFAITriangleSectorRenderer
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Tests FAI triangle sector rendering with interactive point dragging.

**Usage**:

.. code-block:: bash

   ./output/UNIX/bin/RunFAITriangleSectorRenderer

**What it does**:

- Displays FAI triangle sector between two points
- Interactive: drag points A and B to change sector
- Shows sector boundary and area
- Useful for testing FAI triangle calculations and rendering

RunFlightListRenderer
~~~~~~~~~~~~~~~~~~~~~

Tests flight list rendering from a flight log file.

**Usage**:

.. code-block:: bash

   ./output/UNIX/bin/RunFlightListRenderer flights.log

**What it does**:

- Reads flight information from log file
- Renders flight list in a window
- Displays flight dates, times, and statistics
- Useful for testing flight list UI rendering

**Note**: Requires a flight log file (``flights.log``) as argument.

Creating a New Run* Utility
----------------------------

To create a new Run* utility:

1. **Create the source file** in ``test/src/RunYourUtility.cpp``:

.. code-block:: cpp

   // SPDX-License-Identifier: GPL-2.0-or-later
   // Copyright The XCSoar Project
   
   #include "system/Args.hpp"
   #include "DebugReplay.hpp"
   
   #include <stdio.h>
   #include <stdlib.h>
   
   int main(int argc, char **argv)
   {
     Args args(argc, argv, "USAGE");
     DebugReplay *replay = CreateDebugReplay(args);
     if (replay == NULL)
       return EXIT_FAILURE;
     
     args.ExpectEnd();
     
     // Your processing logic here
     
     delete replay;
     return EXIT_SUCCESS;
   }

2. **Add to build system** in ``build/test.mk``:

Find the ``DEBUG_PROGRAM_NAMES`` list and add your utility:

.. code-block:: makefile

   DEBUG_PROGRAM_NAMES += \
     RunYourUtility

Then add build rules:

.. code-block:: makefile

   RUN_YOUR_UTILITY_SOURCES = \
     $(DEBUG_REPLAY_SOURCES) \
     $(TEST_SRC_DIR)/RunYourUtility.cpp
   RUN_YOUR_UTILITY_DEPENDS = $(DEBUG_REPLAY_DEPENDS) GEO MATH UTIL
   $(eval $(call link-program,RunYourUtility,RUN_YOUR_UTILITY))

3. **Build and test**:

.. code-block:: bash

   make output/UNIX/bin/RunYourUtility
   ./output/UNIX/bin/RunYourUtility FLARM test/data/01lz1hq1.igc

Dependencies
------------

Common dependencies for Run* utilities:

- ``DEBUG_REPLAY_SOURCES``: For DebugReplay-based utilities
- ``DEBUG_REPLAY_DEPENDS``: Standard dependencies (GEO, MATH, UTIL, etc.)
- ``TASK``: For task-related utilities
- ``WAYPOINT``: For waypoint-related utilities
- ``AIRSPACE``: For airspace-related utilities
- ``LOOK``: For GUI utilities
- ``DIALOG``: For dialog-based utilities

Check existing utilities in ``build/test.mk`` for examples of dependency
patterns.

Additional Resources
---------------------

- ``test/src/DebugReplay.hpp``: DebugReplay API documentation
- ``test/src/Main.hpp``: GUI utility boilerplate
- ``test/src/DebugPort.hpp``: Device port utilities
- ``test/data/``: Sample flight data files for testing
- ``build/test.mk``: Build system definitions for all utilities

