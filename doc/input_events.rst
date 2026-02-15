Input Events
============

The Input System is deprecated! It is being replaced by a Lua scripting
engine.

Introduction
------------

The Input System is actually a large number of things all bunched into
one.

Primarily it is about giving the user control of what button does what
and when. There is a new concept called Input Mode - this is a the mode
the GUI is in for input. For example, you can click on the info boxes
and you are now in "infobox" mode. Clicking on the map is called
"default". But it doesn’t stop there, you can create a new mode called
anything you like. This may not mean much - but wait till you combine it
with the rest of the features...

Input is not restricted to hardware buttons any more. You can map all
your hardware buttons (currently support for APP1 to APP6, Left, Right,
Up, Down and Enter, although I believe we can do some more) but also any
key code at all. This feature allows those with a built in keyboard to
use any key to map to any function in XCS. Where it comes into real
advantage is in external keyboards. There are a number of bluetooth
devices out there (eg: http://shop.brando.com.hk/btgamepad.php) which
can map each of their buttons to any key code - that key code can then
be mapped to any feature in XCS. You can then add to the hardware
buttons the buttons available to you on external devices. Other inputs
(eg: Serial) are also being looked at - and support is in the code for
that extension.

We are striving towards a platform which is not only easier to use and
more intuitive, but also faster and easier to use in flight as well. As
such, another new feature as part of input is the concept of Button
Labels. Combined with the modes mentioned above, you can create any
arbitrary set of functions to map to any number of buttons. Think about
it like creating a tree, or a multiple level menu.

This produces two benefits that I know will be appreciated by people
with limited inputs. The first is that you can create menus, where by
you press one button to get to the next level (eg: pressing on APP1
brings up AutoZoom, Pan Mode, Full screen on the other buttons. Press
APP1 again and it goes to Terrain, Marker and Auto MacCready. Press APP1
again and the menu is gone) - but more importantly for those with touch
screens and limited buttons, each of these labels can optionally be
assigned a key and you can touch the button area as if it was a button.
This means that we can actually control on a touch screen model the
entire system without buttons - press an area of the screen and the
buttons pop up, click through - change options and more.

The combined features of labels, configurable buttons (including from
external hardware), hierarchical menus (for lack of a better name),
touch screen buttons has allowed us to configure XCS - without recompile
- for an enormous range of hardware, and personal preference. And all
configurable as plane text, simple files. There is no need for a file,
the defaults internally will probably be a combination of a 4 button
bottom system with one button always shown on screen for no/few button
display.

The screen layout - location of the labels - is also totally
configurable - allowing us to vary the layout of buttons depending on
the type of organiser or desired look and feel.

There is a great unexpected benefit in the development of the input
system.

We can execute any number of events attached to an input with only 2
extra lines of code. This worked perfectly. So now we have a basic macro
system, allowing many more events to be attached to a single input
event.

But it doesn’t stop there, this has lead to some more excellent
developments. The idea of Glide Computer Events things like "Maximum
Altitude Reached". Currently we play a sound effect for that. But you
may choose to play a sound, bring up a message box and write to the log
file.

One nice feature of XCS is the ability to change things such as Zoom and
North when Circling. Now you can do so much more. You could choose to
point North, Zoom to 1.0 (rather than a relative change), Turn on Vario
Sounds, Start a timer. When switching back to Cruise mode, you can bring
up the stats box for 30 seconds. The options are limited by your
imagination.

This is also contributing to a major reduction in complex code. We can
move out these complex tests into one centrally, easier to manage
system, reducing bugs and improving maintainability.

Another side benefits of these Macros is User Defined Flight Modes. One
idea was a button which switched to Zoom 1.0, Pan ON, Pan Move to Next
Waypoint. Basically the ability to jump and see the next waypoint. And
in the previous we can change the Input Mode to "ViewWaypoint" - at
which point you can redefine the same button to switch back to your
original settings.

The flexibility of this system comes with only one small price. We can’t
provide an interface within XCS to fully customise all of these near
infinitely variable possibilities. However I believe that is unnecessary
anyway, you are not likely to change these sort of features very often,
and definitely not on the field. That does not mean you can’t, you can
of course edit the plane(sic) text file to change functions.

What this really means is that we can have people in the project helping
and contributing to the customising of XCS, without having to change the
code. This, especially on an open source project is fantastic as it
nicely separates the user interface changes from the highly reliable
part of the code. It also involves people who can develop new interfaces
and functions that are expert gliders but not necessarily programmers.

For information on file formats see :file:`Data/Input/default.xci` and
the web site documentation.

Defaults and Files
------------------

The file in the source :file:`Data/input/default.xci` is used to
generate automatically the C code necessary for the default
configuration. However it is in the exact same format as can be read
in by XCS and therefore can be used literally as a template for a more
complicated file.

When you create your own file, you will need to select it as the Input
File within XCSoar Menu/Config/System/Look/Language,Input/Events. Choose
the custom file you would have previously created, and then restart XCS.

File format
-----------

The file is plain text, with key=value pairs and a blank line to
indicate the end of a record::

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

The record above demonstrates remapping the first hardware key on your
organiser to change Pan to off, Zoom to 1.0 Sounds on, ScreenModes full,
and then a status message to tell you it is done.

Lines are terminated by the stanard DOS newline which is CRLF (Carrage
Return then Line Feed). Records are terminated by an extra new line.

Event order
-----------

Until further work is done on processing, events are actually done in
reverse order - also known as RPN. This is because the events work on
the stack principle. Each one is pushed onto the stack for execution,
and then executed by popping back off the stack. This has reduced
complexity of the code base.

When writing input events, have a look where you put the StatusMessage
and make sure that it is at the top, not the bottom (if you have one).

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

XCSoar now has the concept of Modes. These are an arbitrary string that
associates with where and what XCS is doing.

Note: a mode entry in a record can have multiple entries by using a
space between eg: "infobox menu1 menu2"

List of known modes
~~~~~~~~~~~~~~~~~~~

- ``default``: Really map mode, where you mostly are.
- ``infobox``: An info box has been selected on the screen.
- ``pan``: Pan mode is active.
- ``Menu``: 
- ``*``: Any other arbitrary string.

Mode precedence has been tricky, so instead of solving the problem it is
being worked around. XCS will choose to set a global variable to specify
what mode it thinks it is in. This can then be used by the input code to
decide what to do. This mode could get out of sink with the real world,
and careful checking will be required, but at this stage it seems like
the only sensible option.

The code will review first if an entry exists in the current mode, and
then in the default mode. This allows you to do one of the following
example: Define a default action for button "A" to be "Zoom In" but make
that button increase Bugs value in infobox mode only. You can do this by
making an "default" and a "infobox" entry. You can also put an entry in
for Button "A" for every mode and have complete control.

Special Modes - eg: the level of a menu (Think File vs Edit, vs Tools vs
Help)

have special modes, such as the level of the menu you are at. You press
one button, then another set become available (like pressing menu and
seeing Settings etc). This will be very useful in non-touch screen
models. The menu configuration can then be read from this same file and
configured, allowing any number of levels and any number of
combinations.

The only hard part is what mode to go back to. We need a "Calculate Live
Mode" function - which can be called to calculate the real live mode
(eg: finalglide vs curse) rather than the temporary mode such as Menu,
Special Menu Level, Warning etc.

The label and location values are examples of what can be done here to
allow input button labels to be displayed. What needs to be considered
is a simple way of mapping the locations and the size. In some models it
may be that buttons are 4 across the top of the screen, where as others
it is 3 or 2 or even 6. So both size and location needs to be
considered.

The label itself will go through gettext to allow language translations.

Keys
----

The key type can have the following possible values:

- ``APP1-APP6``: Hardware key on pocket pc
- ``F1-F12``: Standard function keys
- ``LEFT, RIGHT, UP, DOWN, RETURN``: Mapped to arrow 
  keys - joystick on organisers
- ``ESCAPE, MENU, TAB``: function keys
- ``A-Z, 0-9``: and other possible keyboard buttons (case is ignored)

Android only:

- ``BUTTON_R1, BUTTON_R2, BUTTON_L1, BUTTON_L2, BUTTON_A, BUTTON_B, 
  BUTTON_C, BUTTON_X, BUTTON_Y, BUTTON_Z, MEDIA_NEXT, MEDIA_PREVIOUS, 
  MEDIA_PLAY_PAUSE, VOLUME_UP, VOLUME_DOWN``: intended for Bluetooth
  keypads, media controllers

Windows only:

- ``F13-F20``: intended for the Triadis-RemoteStick, as well as for
  expanded keyboards

XXX Review... Input Types

Types:

hardware These are the standard hardware buttons on normal organisers.
Usually these are APP1..6.

keyboard Normal characters on the keyboard (a-z etc)

nmea A sentence received via NMEA stream (either)

virtual Virtual buttons are a new idea, allowing multiple buttons to be
created on screen. These buttons can then be optionally mapped to
physical buttons or to a spot on the screen (probably transparent
buttons over the map).

Modifiers

It is a long term goal of this project to allow modifiers for keys. This
could include one of the following possibilities:

-  Combination presses (although not supported on many devices)

-  Double Click

-  Long Click

Modifiers such as the above will not be supported in the first release.

Glide Computer Events
---------------------

These are automatically triggered events. They work in exactly the same
way, but instead of the user pressing a key, the glide computer triggers
the events.

A simple example is moving from Cruise to Climb mode. We want to zoom
in, change our track up to north up and switch to full screen. You may
also choose to drop a marker with the words "entered thermal". The
choicese are up to your imaginations - the GCE (Glide Computer Events)
allow you to control what happens.

These are represented as ``type=gce`` and ``data=*`` - as listed
below.

``COMMPORT_RESTART``
   The comm port is restarted.

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
   You are at landing.

``STARTUP_REAL``
   First message - this happens at startup of the real XCS.

``STARTUP_SIMULATOR``
   Startup first message. This happens during simulator mode.

``TAKEOFF``
   You have taken off.

``AIRSPACE_NEAR``
   The aircraft has approached an airspace for which warnings are
   enabled.

``AIRSPACE_ENTER``
   The aircraft has entered an airspace for which warnings are
   enabled.
