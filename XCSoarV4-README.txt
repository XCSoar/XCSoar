============================================================================
SUMMARY OF NEW FEATURES AND BUG FIXES SINCE V4.0
============================================================================

TODO
- Enable secondary files check boxes
- "Reset infobox defaults" button

Changes from 4.22 to 4.3

- Speed-to-fly climb mode bug fix
- Thermal band mode fix
- Audio vario sound updates
- Fixed waypoint arrival altitude bug
- New airspace parser, faster and more robust
- New language customisation
- New status message customisation
- Wind algorithm improvements especially at low wind speeds
- Analysis dialog now has page for wind at altitude
- Fixed defaulting to cruise mode when no waypoint active
- Miscellaneous dialog cleanups
- Snail trail colour scales to visible range to make colors more vibrant
- Safe recovery from critical errors when loading files
- Bug fix of polar loading on multiple lines
- Fixed ordering of Menu buttons when using cursor to navigate
- Blanking improvements (prevent timeout advancing when any dialog is active)
- Added Auxiliary infobox display, accessible from APP_KEY1, which now
  toggles through normal (mode-specific) infoboxes, auxiliary infoboxes,
  and fullscreen.
- Settings->Task start line/cylinder labels change dynamically to avoid 
  confusion 
- AutoMcready improvements, fix for overshoot hunting

Changes from 4.21 to 4.22

- Fixed bug when airspace warning display is not refreshed when another
  window overlaps it.
- New "Analysis" pages showing barograph, thermal history and glide polar
- Fixed bug in snail trail, IGC logger update rate
- Additional waypoint file can be specified for competition waypoints
- Fixed font for message box, status dialog
- Minor bugfixes in vario comms thread processing
- Implemented Borgelt B50 vario parsing (untested)
- Improvements to performance and latency of audio
- Terrain cache updates
- File loading improvements
- New wind vector graphics
- New labels with Mc0 arrival height above safety arrival altitude for
  reachable airfields
- Updated aircraft graphics
- Proper units display in dialogs.
- All configuration options now can be expressed in custom units
- New Netto vario infobox
- New dolphin speed-to-fly infobox
- Improved audio vario sounds
- Speed-to-fly director chevrons on right of screen when connected to
  vario with ASI source.
- Fixed rare bugs in McCready calculation
- Fixed bug in terrain rendering, where level of detail was previously
   set at default, and didn't change with zoom.
- Airspace parser made faster, so binary airspace loader now disabled

Changes from 4.2 to 4.21

- Better recovery of bluetooth GPS after switching device off and on
- Marked points appended to file 'xcsoar-marks.txt'
- CDI display configurable
- Settings->Display split into two pages
- Sunset time shown in waypoint details
- AAT and airspace areas drawn below waypoints and topology
- Messagebox enhancements
- MODIS Satelite images now co-located with waypoint file
- Launcher now uninstalls/reinstalls properly.
- Proper spelling of McCready (sorry, Paul!)
- Display blanking automatically after one minute of UI inactivity if in
  battery mode, reactivated with key press
- New GPS status icons, less obtrusive.
- Aircraft disappears when GPS is not connected
- New "Status" summary page from main menu, giving aircraft position,
  nearest waypoint range/bearing, local sunset time, GPS status
- Additional airspace file can be specified for NOTAM airspace updates
- Settings->File page split into two (map data separated off)
- Snail trail toggles between no trail, long trail, and short trail

Summary of new features since v4.0
- Fullscreen mode (app button 1 in map mode); app button 2 now
  toggles snail trail
- Terrain shading via phong model, direction set by wind direction
- Wind vectors multiple for 10 knot increments
- Saving/loading wind to registry
- Time aloft infobox (in Waypoint Group)
- New wind calculation method
- Rendering of airspace with cross-hatches and optional black outline
- Added pilot/aircraft information in logger
- Added "Remove" button on waypoint details task page
- Acknowledge airspace warnings
- Audio settings page
- Graduated snail trail color and thickness
- Abort/resume of tasks
- Added netto vario calculations
- Added smart zooming (zooms back out when waypoint changes if in autozoom)
- Added installer and launcher
- Bring up menu with double click on map window
- Can fly in simulator mode by dragging on screen
- Improved colour selector now displays currently chosen colours
- Added calculation of glider heading from bearing and wind
- Added infoboxes: G-load, time of flight, UTC time, local time, LD to next waypoint
- Adjusted infobox descriptions and titles.
- Added infoboxes: Time to next waypoint, time to task completion

Bug fixes and code improvements
- Sound files are now in the code as resources, so no need for Audio directory
- Filtering of files:
   Waypoints [.txt]
   Airspace [.txt]
   Terrain [.dat]
   Topology [.tpl]
   Polars [.plr]
- Reduced extraneous refresh of navboxes
- Font size improvements
- Second COMM port disabled if set equal to port 1
- Audio thread is suspended when quiet
- Auto McReady now working again
- Improvements to topology handling
- Better terrain color map
- Terrain shading works with elevation files of any resolution.
- Terrain at sea level or below is rendered as water.
- Minor improvements to thread safety
- Larger Menu page buttons
- Fixed McReady speed calculation with zero distance
- Bug fixes by Samuel Gisiger (Airspace not displaying, extraneous
  selection of waypoints at zoom levels)
- Improved map window responsiveness (only re-drawn when necessary, avoiding
  CPU waste of unnecessary re-draws).
- Many hard-wired constants relocated to Sizes.h file
- Waypoint labels have white background so not obscured by terrain
- Labels of topological features now supported
- Fast loading of airspace at startup using binary file
- Wind calculation more reliable
- Fast loading of all startup files

============================================================================
INSTALLATION AND RUNNING
============================================================================

Connect your PDA to a PC and run the Install-XXX.exe on the PC.  This
will start ActiveSync and initiate the install on the PDA.

To run, click on either of the XCSoar icons on the Today screen...


============================================================================
HARDWARE BUTTONS
============================================================================

Hardware buttons and the cursor operate differently in Map mode or InfoBox mode.

Map mode:
- 'App key 1' toggles full screen mode
- 'App key 2' toggles snail trail
- 'App key 3' toggles audio vario
- 'App key 4' marks current location with flag
- 'Enter' cycles through terrain and topology displays
- 'Left' toggles auto zoom
- 'Right' toggles map pan
- 'Up' Zooms in
- 'Down' Zooms out

InfoBox mode (specific to particular InfoBoxes)
- 'Up'/'Down' cursor changes values
- 'Left'/'Right' cursor changes values
  -- Wind speed: Changes wind bearing
  -- Ground speed (simulator): Changes glider track
- 'Enter' toggles special functions
  -- McReady: Toggles auto-mcready in final glide
  -- Waypoint: Brings up waypoint details
  -- LD: Brings up Bugs and Ballast selector
- 'App key 2/3' Cycles between items in InfoBox groups (see below)


============================================================================
INFOBOX GROUPS
============================================================================

InfoBoxes are now grouped logically together so that the user can cycle
through various related values.  These groupings are:
- Altitude group:
  Altitude, Altitude AGL, Terrain Height, Barometric Altitude

- Aircraft info group:
  Bearing, Ground Speed, Track, Airspeed, G loading

- LD group:
  Current LD, Cruise LD, Task LD Finish, LD to Next waypoint

- Vario group:
  Average vario, Last Thermal Average, Last Thermal Gain, Last Thermal Time,
  Thermal Average, Thermal Gain, Vario, Netto Vario

- Wind group:
  Wind speed, Wind bearing

- MacReady group:
  MacReady Setting, MacReady Speed, Percent time climbing, Dolphin speed

- Navigation group:
  Next distance, Next Alt Difference, Next Alt Required, Task Alt Difference,
  Task Alt Required, Task Average Speed, Task Distance, AA Time, AA Max Dist,
  AA Min Dist, AA Max Speed, AA Min Speed

- Waypoint group: 
  Next Waypoint, time flying, local time, UTC time, time to task finish, time to next waypoint


============================================================================
AUDIO
============================================================================

An audio variometer is now available.  It has an intermittent tone
that increases with pitch and rate with positive variometer values.
The quiet part of the tone indicates the average variometer reading.
With negative values, it has a lower pitch with a longer loud tone.
The audio can be switched on and off with the application key 3 when
in map mode.

Support for common electric varios with data output (e.g. Cambridge
302) is planned.

Support for Netto vario is planned.  A second COMM port may be
specified in the Settings->COMM page.  When the second COMM port is
set to the same port as the first, it is disabled.

XCSoar also produces audio messages when turning turnpoints to
indicate when the glider is in the turnpoint sector.  This makes the
sound of a camera shutter.

Sounds are now in three categories:
- Audio vario
- Task sounds (beeps on entering/leaving circling, final glide warning,
  turnpoint notification)
- Mode sounds (user interface sounds when turning on/off options)

All are switchable and settings are saved in registry.

Master volume and audio vario deadband also settable.  The deadband is
a soft volume adjustment around zero.


============================================================================
MARKING POINTS
============================================================================

Marking of the current location is performed with the keypad.  A
history of marks is maintained in a file.  By default, this file gets
over-written at program startup.  The marks appear on the map as small
red flags.


============================================================================
MAIN MENU
============================================================================

The main menu button now disappears when not in use, to free up space on
the map display.  To make it appear, select an InfoBox or double click the
map window.  The menu will disappear again after 10 seconds have elapsed or
if a cursor key is pressed in the InfoBox.


============================================================================
FLIGHT MODES
============================================================================

There are now three flight modes: Cruise, Climb, and Final Glide.  The
mode that is currently active is indicated with a small symbol in the
bottom right corner of the map display.

Each flight mode has its own set of InfoBoxes.  To customise these, bring up 
the Menu, then click 'InfoBoxes locked'.  Then, click on the InfoBox at the
position to be changed to bring up a selector dialog.  The configuration of the
InfoBoxes in each mode can then be modified.  When done, go back into the Menu
and click 'InfoBoxes editable'.


============================================================================
WAYPOINT DETAILS
============================================================================

Information about a waypoint may be brought up by clicking on a
waypoint on the map display, pressing enter in the Waypoint InfoBox,
or from the Details button in the Settings->Task page.

This dialog spans multiple pages.  The first page shows basic waypoint
information such as name, comments, location, elevation, and the type
of waypoint (e.g.  Landable, Turnpoint etc).  If the airfield details
file is available, additional information will be presented.  Usually,
this will contain Enroute Supplement text, such as:
- Runway summary
- Communications
- Remarks
- ICAO code

The second page has task buttons:
- Set home: Sets the selected waypoint as the home field
- Insert here: Inserts the waypoint before the active waypoint in the task.
- Replace: Replaces the active waypoint in the task with the selected one.
- Remove: Removes the active waypoint from the task.
- Final glide to: Cancels the task and sets the selected waypoint as the
  final waypoint.

The third and fourth pages show images from runway diagrams and
satellite imagery if available.  More on this in the next version.


============================================================================
MAP DISPLAY
============================================================================

Infoboxes can be hidden by pressing the App key 1, thereby giving a
full-screen map display.

Pan mode allows the user to drag the screen around to explore beyond
the glider's immediate surrounds.  The user can still zoom in or out when in
pan mode, and select waypoints as usual.

Auto-zoom automatically zooms in when approaching a waypoint to keep
the waypoint at a reasonable screen distance.  The user can still zoom
out if desired.  When auto-zoom is active, an 'A' appears next to the
map scale.  

When a waypoint changes (automatically, via the task selector, or by
manually switching waypoints), autozoom returns the zoom level to what
it was immediately prior to any automatic zooming in it may have
performed.  This has the effect of allowing users to zoom in and out
manually in cruise, and when approaching a waypoint, the system
automatically zooms in.  When passing the waypoint, the system goes
back to the previous cruise zoom level in effect.


There is a facility to have two zoom settings; one when circling, and
one in cruise/final glide.  This is the "Circling zoom" option in the
Settings->Display.  By default, the cruise/final glide zoom is 5 km
and 0.3 km for circling.  When the user zooms in or out, it affects
the current mode's zoom setting only, so when leaving the mode the
previous mode's zoom setting is used.

New icons are used to represent landable waypoints.  These are consistent
with WinPilot:
- Unreachable airfields are purple filled circles  
- Reachable airfields in purple filled circles with a green band

A north arrow is displayed on the top right corner of the map display.
The wind vector is now drawn on top of compass to de-clutter display
near glider.  Winds below 2kt are not displayed.  In stronger winds,
multiple vectors are drawn, in 10 kt increments.  For example, 23
knots will show two long vectors and a short one (two lots of 10 plus
one of 3).

A new waypoint label mode is available.  'Names in task' shows only waypoint
labels if the waypoints are in the current task, or the current home.

The map is capable of displaying terrain elevation contours and vector
topology.  See the section below for more details.

Snail trail uses graduated color to show the glider's vario
measurement.  Green indicates lift and the color varies through grey
(zero lift) to red to indicate sink.  In lift, the trail thickness
increases proportionally to make it easier to identify.


============================================================================
SAFETY HEIGHTS
============================================================================

Three safety heights are defined as:
- Arrival height:  height above the final glide destination for safe arrival
 (typically the circuit height plus some safety margin).
- Break-off height: height above the ground at which the pilot, if descending
 below this height, is expected to abort the task and outland.
- Terrain clearance height: height above terrain for safe clearance on
 final glide.


============================================================================
THERMAL BAND METER
============================================================================

Statistics on climb rates in thermals are collected and displayed in a thermal
band meter.  This is shown above the final glide difference bar on the left
side of the map display.  It is not shown when the glider is above final glide.

The thermal band meter shows a graph, where the vertical axis is
height above the break-off height and is scaled according to the
maximum height achieved.  The horizontal axis is the average climb
rate achieved at a particular height band.  The horizontal axis is
scaled according to the MacReady setting, and an arrow indicating this
setting and the glider's current height is overlaid on the shaded
area.  This scaling and arrow makes it easy to see how the pilot's
MacReady setting compares with achieved thermals and to plan the
desired working height band.


============================================================================
WIND CALCULATION
============================================================================

A new method of estimating wind is now used.  It is based on the
Cumulus program's source code and is far more sophisticated than the
previous method.  It maintains a history of estimates at different
times and altitudes and so provides capability to detect wind at
varying heights.  Quality of estimates, and proximity to the history's
time and altitude is incorporated in the estimation of the current wind speed.

The actual estimation method considers the maximum and minimum ground
speed achieved within a single circle, and considers the bearing at
each point to work out a quality of fit.  The first few circles have
lower quality, reflecting that usually the glider is centering lift at
those times.  

This new method does not suffer the rather extreme errors involved in
the previous model's method, which looked at drift during the entire
circling phase.  However, it has not been extensively tested yet and
so user reports will be appreciated.

XCSoar loads the previous saved wind settings at startup.  To save the
current wind setting, press the Enter cursor with a Wind InfoBox
active.


============================================================================
PROFILES
============================================================================

The saving and loading of profiles now includes files used by XCSoar:
 -- Airspace
 -- Airfield details
 -- Waypoints
 -- Terrain
 -- Topology
 -- Winpilot polar


============================================================================
DATA LOGGING
============================================================================

IGC logging is now improved so that the logger outputs at higher rates 
(one per second) when in the turnpoint sector.


============================================================================
GLIDE COMPUTER
============================================================================

Auto MacRready is selected by pressing 'Enter' on the MacReady InfoBox
when in Final Glide flight mode.  It adjusts the MacReady setting so
excess height is converted to higher speed and vica versa.  A mode
icon, showing a green and red triangle, appears in the bottom right
corner of the map window when active.

The glide computer now accounts for wind drift during circling to
provide a 'best thermal track' vector, which indicates the track the
glider should follow during cruise such that it will arrive at the
waypoint in minimum time.  This vector is displayed on the map as a blue
arrow.  When wind is negligable, or in final glide mode, this arrow will
point along the black line indicating the track to waypoint.


============================================================================
FINAL GLIDE THROUGH TERRAIN
============================================================================

The final glide path is checked for whether the glider clears terrain by
the terrain clearance height.  If clearance is not attained, a red cross
appears on the map at the point where the violation occurs.

A reachable glide footprint is displayed on the map display as a dashed line,
indicating where the glider would descend through the terrain clearance height.
This footprint is calculated for tracks extending 180 degrees around the glider.
The reachable glide footprint is useful in assessing range with respect to
topology when searching low for lift, and when flying in mountainous areas.


============================================================================
POLARS
============================================================================

WinPilot polar files may be used.  The aircraft type should be set to 'WinPilot File'
in the Settings->Polar page, where it is the particular 
file name is also defined.  

The user can specify a maximum manoeuvering speed, which limits the
speed-to-fly in MacReady calculations to realistic values.  This is
specified in m/s.


============================================================================
TERRAIN AND TOPOLOGY
============================================================================

The map display can show digital terrain elevation and vector topology.

The terrain data is raster data.  A new technique for cacheing the terrain
database reduces the file access overhead, allowing very large
terrain files to be used.  The terrain elevation is shown as colour contours
correspoinding to the following height map:
- dark green
- light green
- light brown
- grey
- white
- blue-white

Terrain is phong-shaded to improve visibility.  Currently the shading
is set up so that the virtual lighting position is the wind bearing,
thus brighter areas are on the upwind side of hills and dark areas in
the lee of the hill.  Support for a sun ephemeris is underway.

Support for user-defined contours is planned.

The topology data uses ERSI Shapefiles of the following types:
- Points
- Lines
- Areas
The topology file (extension .tpl) defines which features are to be
displayed, their color, maximum zoom visibility, icons, and labelling.
Details on the file format will be provided in a separate document.

The files may be generated from the freely available VMAP0 format, converted
with the freely available GDAL/OGR utility.

Shape files are also cached and loaded on demand, allowing large files to
be used.



============================================================================
AIRSPACE
============================================================================

Hitting the enter cursor key while in map mode will dismiss any raised
airspace warnings for a period of time specified in the Acknowledgement
time in Settings->Airspace Warnings.


============================================================================
TASKING
============================================================================

From the main menu, the user can Abort or Resume a task.  When a task
is aborted, the nearest landable waypoints are used as a virtual task,
sorted by distance.  The map window shows the vectors from the
aircraft to each close landable waypoint instead of the task.  

If the active waypoint prior to the abort is in the close landable
waypoint list, it will be the active waypoint in the aborted mode.
Otherwise, the active waypoint will be the closest waypoint.  'Close'
in this sense means the waypoint with the highest arrival height,
assuming flight with zero speed ring setting.

After a task has been aborted, the user can still switch
between waypoints --- this allows the user to head towards a preferred
landing site.

Resuming a task restores the previously active task and active
waypoint.

By default now, aborting a task always sets the active waypoint to the
closest landable field.
