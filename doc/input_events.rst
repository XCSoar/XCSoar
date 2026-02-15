Input Events
============

.. note::

   The Input Event system is gradually being supplemented by
   :doc:`Lua scripting <lua>`. New automation is best written in Lua;
   the ``.xci`` format described here remains fully supported for
   button and key mapping.

Overview
--------

The Input Event system maps hardware buttons, keyboard keys,
touch-screen labels, and external input devices to XCSoar actions.
Bindings are defined in plain-text ``.xci`` files and can be changed
without recompiling.

Key concepts:

- **Events** -- actions XCSoar can perform (zoom, pan, open a dialog,
  change a setting, etc.). A single input can trigger multiple events
  in sequence.
- **Modes** -- named states (``default``, ``infobox``, ``pan``, or
  any custom name) that determine which set of bindings is active.
  This allows the same button to do different things depending on
  context, and enables hierarchical menus.
- **Labels** -- on-screen button labels tied to modes, giving
  touch-screen users the same access as hardware-button users.
- **Glide Computer Events** -- events triggered automatically by the
  glide computer (e.g. flight-mode changes, takeoff, landing) rather
  than by user input.

Default configuration
---------------------

The source file :file:`Data/Input/default.xci` contains the built-in
bindings. It is compiled into XCSoar and also serves as a template
for custom files.

To use a custom file, go to
**Menu > Config > System > Look > Language, Input > Events**, select
your ``.xci`` file, and restart XCSoar.

File format
-----------

A ``.xci`` file is plain text with CRLF line endings. Each record is
a group of ``key=value`` lines separated from the next record by a
blank line::

 mode=default
 type=key
 data=APP1
 event=StatusMessage My favorite settings are done
 event=ScreenModes full
 event=Sounds on
 event=Zoom 1.0
 event=Pan off
 label=My Prefs
 location=1

This example remaps the APP1 hardware key to disable pan, set zoom to
1.0, enable vario sounds, switch to full-screen mode, and display a
confirmation message.

Event execution order
---------------------

Events within a record are executed in reverse order (last listed
runs first). Place ``StatusMessage`` lines at the top of the event
list so they execute last and appear after all other actions have
completed.

Event list
----------

.. list-table::
 :widths: 20 80
 :header-rows: 1

 * - Event
   - Description
 * - ``AbortTask``
   - Controls task abort and resume. Possible arguments: ``abort``
     (aborts the task), ``resume`` (resumes an aborted task),
     ``toggle`` (toggles between abort and resume), ``show``
     (displays the current task abort status).
 * - ``AddWaypoint``
   - Opens the waypoint editor to create a new waypoint at the
     current location. Use ``goto`` to add a temporary goto point
     at the current map position.
 * - ``AdjustForecastTemperature``
   - Adjusts the maximum ground temperature for convection forecast.
     Possible arguments: ``+`` (increase by one degree), ``-``
     (decrease by one degree), ``show`` (display current value).
 * - ``AdjustVarioFilter``
   - Adjusts the Vega variometer filter time constant and
     diagnostics. Possible arguments: ``slow``, ``medium``, ``fast``
     (filter speed), ``statistics``, ``diagnostics``, ``psraw``,
     ``switch`` (diagnostic modes), ``democlimb``, ``demostf``
     (demo modes), ``zero`` (zero airspeed offset), ``save`` (save
     config to EEPROM).
 * - ``AdjustWaypoint``
   - Adjusts the active waypoint of the task. Possible arguments:
     ``next`` (next waypoint), ``previous`` (previous waypoint),
     ``nextwrap`` (next with wrap), ``previouswrap`` (previous with
     wrap), ``nextarm`` (arm-sensitive next), ``previousarm``
     (arm-sensitive previous).
 * - ``AirSpace``
   - Toggles airspace display on the map. Possible arguments:
     ``toggle``, ``on``, ``off``, ``show``, ``list`` (opens
     the airspace list dialog).
 * - ``AirspaceDisplayMode``
   - Controls the airspace display filter mode. Possible arguments:
     ``all`` (show all), ``clip`` (clip to altitude), ``auto``
     (automatic), ``below`` (show all below), ``off`` (hide all).
 * - ``AirspaceWarnings``
   - Opens the airspace warnings dialog.
 * - ``Analysis``
   - Displays the analysis/statistics dialog.
 * - ``ArmAdvance``
   - Controls waypoint advance trigger arming. Possible arguments:
     ``on`` (arm), ``off`` (disarm), ``toggle``, ``show``
     (display current state).
 * - ``AudioDeadband``
   - Adjusts the audio deadband of internal vario sounds. Possible
     arguments: ``+`` (increase deadband), ``-`` (decrease
     deadband).
 * - ``AutoLogger``
   - Same as ``Logger``, but only activates if the auto-logger
     setting is enabled.
 * - ``Ballast``
   - Adjusts the glider ballast setting. Possible arguments: ``up``
     (increase by 10%), ``down`` (decrease by 10%), ``max`` (100%),
     ``min`` (0%), ``show`` (display current value).
 * - ``Beep``
   - Plays a beep sound.
 * - ``Brightness``
   - Adjusts screen brightness. (Not currently implemented.)
 * - ``Bugs``
   - Adjusts the bugs/degradation setting for glider performance.
     Possible arguments: ``up`` (increase by 10%), ``down``
     (decrease by 10%), ``max`` (clean), ``min`` (worst), ``show``
     (display current value).
 * - ``Calculator``
   - Opens the task manager dialog.
 * - ``Checklist``
   - Displays the checklist dialog.
 * - ``ClearAirspaceWarnings``
   - Acknowledges all active airspace warnings.
 * - ``ClearStatusMessages``
   - Acknowledges and clears all status message warnings.
 * - ``Credits``
   - Displays the credits dialog.
 * - ``DeclutterLabels``
   - Controls waypoint label display filtering. Possible arguments:
     ``toggle`` (cycle through modes), ``show`` (display current
     mode), ``all``, ``task+landables``, ``task``, ``none``,
     ``task+airfields``.
 * - ``Device``
   - Device management. Possible arguments: ``list`` (opens the
     device list dialog).
 * - ``ExchangeFrequencies``
   - Swaps the active and standby radio frequencies.
 * - ``Exit``
   - Close XCSoar.
 * - ``FileManager``
   - Opens the file manager dialog.
 * - ``FlarmDetails``
   - Opens the FLARM traffic list dialog.
 * - ``FLARMRadar``
   - Controls the FLARM radar gauge display. Use ``ForceToggle`` to
     force the radar on/off; without arguments, toggles suppression
     of the automatic radar display.
 * - ``FlarmTraffic``
   - Opens the full-screen FLARM traffic radar page.
 * - ``GestureHelp``
   - Opens the gesture help dialog showing available touch gestures.
 * - ``GotoLookup``
   - Opens the waypoint selector and sets the selected waypoint as
     a goto target.
 * - ``LockScreen``
   - Displays the screen lock dialog.
 * - ``Logger C``
   - Activates the internal IGC logger.

     ``start``: starts the logger

     ``start ask``: starts the logger after asking the user to
     confirm

     ``stop``: stops the logger

     ``stop ask``: stops the logger after asking the user to confirm

     ``toggle``: toggles between on and off

     ``toggle ask``: toggles between on and off, asking the user to
     confirm

     ``show``: displays a status message indicating whether the
     logger is active

     ``nmea``: turns on and off NMEA logging

     ``note``: the text following the 'note' characters is added to
     the log file
 * - ``MacCready``
   - Adjusts the MacCready setting. Possible arguments: ``up``
     (increase), ``down`` (decrease), ``auto toggle``, ``auto on``,
     ``auto off``, ``auto show``, ``show`` (display current value).
 * - ``MainMenu``
   - Opens the main menu.
 * - ``MarkLocation``
   - Marks the current location and creates a user waypoint marker.
     Use ``reset`` to erase all user markers.
 * - ``Mode M``
   - Sets the current input event mode. The argument is the label
     of the mode to activate (e.g. ``default``, ``Menu``).
 * - ``NearestAirspaceDetails``
   - If airspace warnings are active, opens the airspace warnings
     dialog. Otherwise, finds the nearest airspace (within 30
     minutes flight time) and shows its details.
 * - ``NearestMapItems``
   - Displays the map item list dialog for items near the current
     map position.
 * - ``NearestWaypointDetails``
   - Displays details for the nearest waypoint to the current map
     position.
 * - ``Null``
   - Does nothing. Can be used to override default functionality
     for a key/button.
 * - ``OrientationCircling``
   - Sets the map orientation for circling mode. Possible arguments:
     ``northup``, ``trackup``, ``headingup``, ``targetup``,
     ``windup``.
 * - ``OrientationCruise``
   - Sets the map orientation for cruise mode. Possible arguments:
     ``northup``, ``trackup``, ``headingup``, ``targetup``,
     ``windup``.
 * - ``Page``
   - Controls page navigation. Possible arguments: ``restore``
     (restore the previously active page).
 * - ``Pan [P]``
   - Control pan mode. Possible arguments: ``on`` (enable pan),
     ``off`` (disable pan), ``toggle``, ``up``, ``down``, ``left``,
     ``right``.
 * - ``PilotEvent``
   - Announces a Pilot Event (PEV). Sets the PEV start time window
     based on the current task's start constraints, logs the event
     to the IGC file, and notifies connected devices.
 * - ``PlaySound S``
   - Play the specified sound resource.
 * - ``ProfileLoad PATH``
   - Loads a profile from the specified file.
 * - ``ProfileSave PATH``
   - Saves the current profile to the specified file.
 * - ``QuickGuide``
   - Opens the Quick Guide dialog.
 * - ``QuickMenu``
   - Opens the quick menu dialog.
 * - ``RepeatStatusMessage``
   - Repeats the last status message. If pressed repeatedly, will
     repeat previous status messages.
 * - ``ResetTask``
   - Resets the current task.
 * - ``Run PATH``
   - Runs an external program of the specified filename. Note that
     XCSoar will wait until this program exits.
 * - ``RunLuaFile [PATH]``
   - Runs a Lua script. If a ``.lua`` filename is given, runs that
     file from the ``lua/`` directory. If no argument is given,
     opens a file picker to select a Lua script.
 * - ``ScreenModes M``
   - Set the screen mode. Possible arguments: ``normal``,
     ``auxilary``, ``toggleauxiliary``, ``full``, ``togglefull``,
     ``toggle``, ``show``, ``previous``, ``next``.
 * - ``SendNMEA``
   - Sends a user-defined NMEA string to all Vega variometers. The
     string is automatically prefixed with ``$`` and appended with
     the checksum.
 * - ``SendNMEAPort1``
   - Sends a user-defined NMEA string to the device on port 1.
 * - ``SendNMEAPort2``
   - Sends a user-defined NMEA string to the device on port 2.
 * - ``Setup D``
   - Activates configuration and setting dialogs.

     ``Basic``: Basic settings (QNH/Bugs/Ballast/MaxTemperature)

     ``Wind``: Wind settings

     ``Task``: Task editor

     ``Airspace``: Airspace filter settings

     ``Replay``: IGC replay dialog

     ``System``, ``Weather``, ``Switches``, ``Teamcode``,
     ``Target``, ``Plane``, ``Profile``, ``Alternates``
 * - ``SnailTrail S``
   - Change snail trail setting. Possible arguments: ``off``,
     ``short``, ``long``, ``full``, ``toggle``, ``show``.
 * - ``Sounds S``
   - Change vario sounds. Possible arguments: ``toggle``, ``on``,
     ``off``, ``show``.
 * - ``Status``
   - Displays one of the status dialogs. Possible arguments:
     ``system``, ``aircraft``, ``task``.
 * - ``StatusMessage MSG``
   - Display the specified status message text.
 * - ``TaskLoad PATH``
   - Loads a task from the specified file.
 * - ``TaskSave PATH``
   - Saves the current task to the specified file.
 * - ``TaskTransition``
   - Displays task transition messages. Possible arguments:
     ``start`` (show start statistics), ``next`` (next turnpoint
     message), ``finish`` (task finished message).
 * - ``TerrainTopography``
   - Controls terrain and topography display. Possible arguments:
     ``terrain toggle``, ``terrain on``, ``terrain off``,
     ``topography toggle``, ``topography on``, ``topography off``,
     ``toggle`` (cycle all four states), ``show``.
 * - ``TerrainTopology``
   - Deprecated alias for ``TerrainTopography``.
 * - ``ThermalAssistant``
   - Opens the thermal assistant page.
 * - ``Traffic``
   - Controls the FLARM traffic radar widget. Possible arguments:
     ``show`` (open radar page), ``zoom auto toggle``, ``zoom in``,
     ``zoom out``, ``northup toggle``, ``details`` (open selected
     traffic details), ``label toggle`` (cycle data labels).
 * - ``UploadIGCFile``
   - Opens a file picker to select an IGC file and uploads it to
     WeGlide.
 * - ``UserDisplayModeForce M``
   - Forces a display mode. Possible arguments: ``unforce``,
     ``forceclimb``, ``forcecruise``, ``forcefinal``.
 * - ``WaypointDetails W``
   - Displays airfield/waypoint details.

     ``current``: the current active waypoint

     ``select``: brings up the waypoint selector; if the user
     selects a waypoint, the details dialog is shown.
 * - ``WaypointEditor``
   - Opens the waypoint editor/configuration dialog.
 * - ``Weather``
   - Opens the weather dialog.
 * - ``Zoom Z``
   - Controls map zoom. Possible arguments: ``auto toggle``,
     ``auto on``, ``auto off``, ``auto show``, ``in``, ``out``,
     ``+``, ``++``, ``-``, ``--``, ``circlezoom toggle``,
     ``circlezoom on``, ``circlezoom off``, ``circlezoom show``,
     or a numeric value to set a specific zoom scale.

Modes
-----

A mode is a named state that determines which bindings are active.
When XCSoar looks up a binding for an input, it first checks the
current mode and then falls back to ``default``. This lets you
override a key for a specific context without affecting its behaviour
elsewhere.

A record's ``mode`` field can list multiple modes separated by
spaces (e.g. ``mode=infobox menu1 menu2``).

Built-in modes
~~~~~~~~~~~~~~

- ``default`` -- the normal map view; active most of the time.
- ``infobox`` -- an InfoBox has been selected.
- ``pan`` -- pan mode is active.
- ``Menu`` -- a menu level is open.

You may define any additional mode names to build custom menu
hierarchies.

Labels
~~~~~~

Each record can include a ``label`` and ``location`` to display an
on-screen button. Labels are passed through ``gettext`` for
translation. The number of label slots and their layout depend on the
device's screen size.

Keys
----

The ``data`` field for ``type=key`` records accepts the following
values:

- ``APP1`` -- ``APP6``: hardware application keys
- ``F1`` -- ``F12``: standard function keys
- ``LEFT``, ``RIGHT``, ``UP``, ``DOWN``, ``RETURN``: directional and
  enter keys
- ``ESCAPE``, ``MENU``, ``TAB``: navigation keys
- ``A`` -- ``Z``, ``0`` -- ``9``: alphanumeric keys (case-insensitive)

Android:

- ``BUTTON_R1``, ``BUTTON_R2``, ``BUTTON_L1``, ``BUTTON_L2``,
  ``BUTTON_A``, ``BUTTON_B``, ``BUTTON_C``, ``BUTTON_X``,
  ``BUTTON_Y``, ``BUTTON_Z``, ``MEDIA_NEXT``, ``MEDIA_PREVIOUS``,
  ``MEDIA_PLAY_PAUSE``, ``VOLUME_UP``, ``VOLUME_DOWN``: gamepad and
  media keys for Bluetooth controllers

Windows:

- ``F13`` -- ``F20``: extended function keys (e.g. Triadis
  RemoteStick)

Input types
~~~~~~~~~~~

- ``key`` -- a hardware or keyboard key press.
- ``gce`` -- a Glide Computer Event (see below).
- ``nmea`` -- a sentence received on an NMEA port.

Glide Computer Events
---------------------

Glide Computer Events (GCEs) are triggered automatically by the glide
computer rather than by user input. They are defined with
``type=gce`` and the event name in the ``data`` field. You can attach
the same actions to a GCE as to any other input -- for example, play
a sound, display a message, or change the zoom level when entering a
thermal.

``COMMPORT_RESTART``
   The comm port has been restarted.

``FLIGHTMODE_CLIMB``
   The flight mode has switched to "climb".

``FLIGHTMODE_CRUISE``
   The flight mode has switched to "cruise".

``FLIGHTMODE_FINALGLIDE``
   The flight mode has switched to "final glide".

``GPS_CONNECTION_WAIT``
   Waiting for the GPS connection.

``GPS_FIX_WAIT``
   Waiting for a valid GPS fix.

``HEIGHT_MAX``
   Maximum height reached for this trip.

``LANDING``
   Landing detected.

``STARTUP_REAL``
   XCSoar has started in normal mode.

``STARTUP_SIMULATOR``
   XCSoar has started in simulator mode.

``TAKEOFF``
   Takeoff detected.

``AIRSPACE_NEAR``
   The aircraft has approached an airspace for which warnings are
   enabled.

``AIRSPACE_ENTER``
   The aircraft has entered an airspace for which warnings are
   enabled.
