#############
Lua Scripting
#############

Starting with version 7.0, XCSoar can be extended using `Lua
<http://www.lua.org/>`__ scripts.

Lua is a language that is easy to learn, powerful enough for XCSoar,
and lightweight; the interpreter library weighs just 200 kB. Lua is a common
choice for integrated scripting languages.


Learning Lua
============

.. code-block:: lua

 print("Hello World")

This manual will not attempt to teach you basic Lua. There are enough
resources on the internet, for example:

-  `Lua 5.4 Reference Manual <http://www.lua.org/manual/5.4/>`__

-  `Programming in Lua <http://www.lua.org/pil/contents.html>`__, a book
   on Lua

-  `Tutorial Directory <http://lua-users.org/wiki/TutorialDirectory>`__
   on the lua-users wiki

-  `Wikipedia <https://en.wikipedia.org/wiki/Lua_%28programming_language%29>`__

Just to get you started from here, here’s some more example code:

.. code-block:: lua

 -- comment starts with a double hyphen
 
 --[[
 multi
 line
 comment
 ]]--
 
 i = 42
 if i > 1 then
    print("i=" .. i)
 elseif i == 0 then
    print("zero")
 else
    error("negative")
 end
 
 a = {1, 'a', 3.14}
 print(a[2])
 
 function f(a, b)
    return a * b
 end
 print(f(2, 3))


Running Lua
===========

The directory :file:`XCSoarData/lua/` may contain Lua scripts (:file:`*.lua`).
The directory :file:`XCSoarData/lua/lib/` may contain Lua libraries to be
loaded with ``require``.

After startup, XCSoar starts the script :file:`init.lua` (if it
exists).

The *InputEvent* ``RunLuaFile`` can be used to start additional
scripts. If no parameter is given, the user is asked to choose a file.
Note that the *InputEvent* subsystem is deprecated and will be removed
once Lua support is complete.

As long as a Lua script runs, the XCSoar user interface is blocked. Be careful
not to write scripts that loop forever.

Once the Lua script finishes, the Lua interpreter is shut down –
unless the script has registered a callback (e.g. a ``timer``). In
that case, the Lua script stays resident until it unregisters all
callbacks (or until XCSoar quits or the user stops the script
explicitly).

Running Lua on the Command Line
-------------------------------

You can experiment with XCSoar Lua scripts with the :program:`RunLua`
command-line tool, e.g.::

  make output/UNIX/bin/RunLua
  ./output/UNIX/bin/RunLua test/lua/geo.lua

Most libraries are missing here (everything specific to a full
XCSoar), but some very basic things can be tested here, for example
the HTTP client and the geo library (class ``GeoPoint``). Being a
small non-interactive command-line program, it is also easy to debug
with :program:`gdb` and :program:`valgrind`.


Lua Standard Libraries
======================

XCSoar enables the following Lua standard libraries:

-  ``package``

-  ``table``

-  ``string``

-  ``math``

Lua’s ``print()`` function writes to the XCSoar log file
(:file:`XCSoarData/xcsoar.log`).

The ``error()`` function aborts the Lua script and reports the specified
error message to the user.

XCSoar adds another function to the root namespace: ``alert()``. It
shows a dialog with the specified message, and returns as soon as the
user has closed the dialog. This function is experimental, and may
disappear or be renamed at any time. Most importantly: do not abuse
it, as it may annoy the user.

XCSoar's Lua API
================

The package/namespace ``xcsoar`` provides access to XCSoar. It
contains the following names:

.. list-table::
 :widths: 20 80
 :header-rows: 1

 * - Name
   - Description
 * - ``VERSION``
   - The XCSoar version number, for example ``7.0``.
 * - ``GeoPoint``
   - A class which describes a geodetic location on earth's surface.
 * - ``blackboard``
   - Access to sensor data. See :ref:`lua.blackboard`.
 * - ``map``
   - The map view. See :ref:`lua.map`.
 * - ``airspace``
   - Access to airspace data. See :ref:`lua.airspace`.
 * - ``task``
   - Access to task data. See :ref:`lua.task`.
 * - ``settings``
   - Access to XCSoar's settings. See :ref:`lua.settings`.
 * - ``wind``
   - Access to wind data and settings. See :ref:`lua.wind`.
 * - ``logger``
   - Access to logger settings. See :ref:`lua.logger`.
 * - ``tracking``
   - Access to tracking settings. See :ref:`lua.tracking`.
 * - ``replay``
   - Access to replay system. See :ref:`lua.replay`.
 * - ``timer``
   - Class for scheduling periodic callbacks. See :ref:`lua.timer`.
 * - ``http``
   - HTTP client. See :ref:`lua.http`.
 * - ``share_text(text)``
   - Deliver plain text data to somebody; the user will be asked to
     pick a recipient (Android only).

.. _lua.blackboard:

Blackboard
----------

The blackboard provides access to sensor data, such as GPS location.

The following attributes are provided by ``xcsoar.blackboard``:

.. list-table::
 :widths: 20 80
 :header-rows: 1

 * - Name
   - Description
 * - ``clock``
   - A monotonic wall clock time [s], with an undefined reference.
 * - ``time``
   - A wall clock time [s], since midnight (UTC) of the day the flight
     started. Not strictly monotonic (can warp under certain
     circumstances).
 * - ``date_time_utc``
   - A `date table <https://www.lua.org/pil/22.1.html>`__ describing
     the current date and time (UTC), preferably from the GPS. Not
     strictly monotonic (can warp under certain circumstances).
 * - ``location``
   - The current location (table with keys ``longitude`` and
     ``latitude`` [°]) according to GPS.
 * - ``altitude``
   - The current altitude above MSL [m].
 * - ``altitude_agl``
   - The current altitude above ground [m].
 * - ``track``
   - The current flying direction above ground [°].
 * - ``ground_speed``
   - The aircraft speed relative to the ground [:math:`m/s`].
 * - ``air_speed``
   - The true airspeed [:math:`m/s`].
 * - ``bank_angle``
   - The bank angle [°].
 * - ``pitch_angle``
   - The pitch angle [°].
 * - ``heading``
   - The current magnetic heading in [°].
 * - ``g_load``
   - The current g-load [:math:`kg/m^2`].
 * - ``static_pressure``
   - The static pressure [Pascal].
 * - ``pitot_pressure``
   - The pitot pressure [Pascal].
 * - ``dynamic_pressure``
   - The dynamic pressure [Pascal].
 * - ``temperature``
   - The current temperature [Kelvin].
 * - ``humidity``
   - The current relative humidity [%].
 * - ``voltage``
   - The external battery voltage [V].
 * - ``battery_level``
   - The internal battery-level [%].
 * - ``noncomp_vario``
   - The non-compensated vertical speed [:math:`m/s`].
 * - ``total_energy_vario``
   - The total-energy-compensated vertical speed [:math:`m/s`].
 * - ``netto_vario``
   - The netto variometer value [:math:`m/s`].

Any of these (except for ``clock``) may be ``nil`` if its value is not
known, e.g. if there is no GPS fix.

.. _lua.map:

Map
---

The map provides access to XCSoar’s map view.

The following attributes are provided by ``xcsoar.map``:

.. list-table::
 :widths: 20 80
 :header-rows: 1

 * - Name
   - Description
 * - ``location``
   - The current reference location (may be aircraft location or
     pan location).
 * - ``is_panning``
   - Gives back if the panning mode is active at the moment.
 * - ``enterpan()``
   - Activates the panning mode.
 * - ``disablepan()``
   - Disables the panning mode.
 * - ``leavepan()``
   - Leaves the panning mode.
 * - ``panto(latitude, longitude)``
   - Pans to the given location.
 * - ``pancursor(dx, dy)``
   - Pans the cursor by ``dx`` and ``dy``.
 * - ``zoom(factor)``
   - Zooms the map, factor -2 to 2.
 * - ``next()``
   - Opens the next page.
 * - ``prev()``
   - Opens the previous page.
 * - ``show()``
   - Show the map; disable thermal assistant or other
     widgets replacing the map view.

.. _lua.airspace:

Airspace
--------

The Airspace provides access to airspace data, such as name / distance
to the next airspace.

The following attributes are provided by ``xcsoar.airspace``:

.. list-table::
 :widths: 20 80
 :header-rows: 1

 * - Name
   - Description
 * - ``nearest_vertical_distance``
   - The vertical distance to the next airspace [m].
 * - ``nearest_vertical_name``
   - The name of the next vertical airspace.
 * - ``nearest_horizontal_distance``
   - The horizontal distance to the next airspace [m].
 * - ``nearest_horizontal_name``
   - The name of the next horizontal airspace.

.. _lua.task:

Task
----

The Task provides access to task data such as distances / bearing to the
next waypoint.

The following attributes are provided by ``xcsoar.task``:

.. list-table::
 :widths: 20 80
 :header-rows: 1

 * - Name
   - Description
 * - ``bearing``
   - The true bearing to the next waypoint [°]. For AAT tasks,
     this is the true bearing to the target within the AAT sector.
 * - ``bearing_diff``
   - The difference between the glider's track bearing, to the bearing
     of the next waypoint, or for AAT tasks, to the bearing to the
     target within the AAT sector [°].
 * - ``radial``
   - The true bearing from the next waypoint to your
     position. [°].
 * - ``next_distance``
   - The distance to the currently selected waypoint [m]. For AAT
     tasks, this is the distance to the target within the AAT sector.
 * - ``next_distance_nominal``
   - The distance to the currently selected waypoint [m]. For AAT
     tasks, this is the distance to the origin of the AAT sector.
 * - ``next_ete``
   - Estimated time required to reach next waypoint, assuming
     performance of ideal MacCready cruise/climb cycle [s].
 * - ``next_eta``
   - Estimated arrival local time at next waypoint,
     assuming performance of ideal MacCready cruise/climb cycle.
 * - ``next_altitude_diff``
   - Arrival altitude at the next waypoint relative to the safety
     arrival height [m].
 * - ``nextmc0_altitude_diff``
   - Arrival altitude at the next waypoint with MC 0 setting, relative
     to the safety arrival height [m].
 * - ``next_altitude_require``
   - Additional altitude required to reach the next
     turnpoint [m].
 * - ``next_altitude_arrival``
   - Absolute arrival height at the next waypoint in final glide [m].
 * - ``next_gr``
   - The required glide ratio over ground to reach the next waypoint,
     given by the distance to the next waypoint divided by the height
     required to arrive at the safety arrival height.
 * - ``final_distance``
   - Distance to finish around remaining turn points [m].
 * - ``final_ete``
   - Estimated time required to complete task, assuming performance of
     ideal MacCready cruise/climb cycle [s].
 * - ``final_eta``
   - Estimated arrival local time at task completion, assuming
     performance of ideal MacCready cruise/climb cycle.
 * - ``final_altitude_diff``
   - Arrival altitude at the final task turn point relative to the
     safety arrival height [m].
 * - ``finalmc0_altitude_diff``
   - Arrival altitude at the final task turn point, with MC 0 setting,
     relative to the safety arrival height [m].
 * - ``final_altitude_require``
   - Additional altitude required to finish
     the task [m].
 * - ``task_speed``
   - Average cross country speed while on the current task, not
     compensated for altitude [:math:`m/s`].
 * - ``task_speed_achieved``
   - Achieved cross country speed while on the current task,
     compensated for altitude. Equivalent to Pirker cross country
     speed remaining [:math:`m/s`].
 * - ``task_speed_instant``
   - Instantaneous cross country speed while on the current task,
     compensated for altitude. Equivalent to instantaneous Pirker
     cross country speed [:math:`m/s`].
 * - ``task_speed_hour``
   - Average cross country speed while on the current task over the
     last hour, not compensated for altitude [:math:`m/s`].
 * - ``final_gr``
   - The required glide ratio over the ground to finish the task,
     given by the distance to go divided by the height required to
     arrive at the safety arrival height.
 * - ``aat_time``
   - Assigned Area Task time remaining [s].
 * - ``aat_time_delta``
   - Difference between estimated task time and
     AAT miminum time [s].
 * - ``aat_distance``
   - Assigned Area Task distance around target points
     for remainder of task [m].
 * - ``aat_distance_max``
   - Assigned Area Task maximum distance possible for remainder of
     task [m].
 * - ``aat_distance_min``
   - Assigned Area Task minimum distance possible
     for remainder of task [m].
 * - ``aat_speed``
   - Assigned Area Task average speed achievable around
     target points remaining in minimum AAT time [:math:`m/s`].
 * - ``aat_speed_max``
   - Assigned Area Task average speed achievable if flying maximum
     possible distance remaining in minimum AAT time [:math:`m/s`].
 * - ``aat_speed_min``
   - Assigned Area Task average spped achievable if flying minimum
     possible distance remaining in minimum AAT time [:math:`m/s`].
 * - ``time_under_max_height``
   - The contiguous period the plane has been below the task start
     max. height [s].
 * - ``next_etevmg``
   - Estimated time required to reach next waypoint, assuming current
     ground speed is maintained [s].
 * - ``final_etevmg``
   - Estimated time required to complete task,
     assuming current ground speed is maintained [s].
 * - ``cruise_efficiency``
   - Efficiency of cruise. 1 indicates perfect MacCready performance.

.. _lua.settings:

Settings
--------

The Settings provides access to XCSoar's settings, such as MC value.

The following attributes are provided by ``xcsoar.settings``:

.. list-table::
 :widths: 30 70
 :header-rows: 1

 * - Name
   - Description
 * - ``mc``
   - The current set MacCready Value [:math:`m/s`].
 * - ``bugs``
   - The current used bug settings in terms of polar degradation.
     1 means "clean".
 * - ``wingload``
   - The current wingload [:math:`kg/m^2`].
 * - ``ballast``
   - Ballast of the glider. 0 means no ballst, 0.3 means 30% of the
     maximum ballast the glider can carry.
 * - ``qnh``
   - Area pressure for barometric altimeter calibration [Pascal].
 * - ``max_temp``
   - The forecast ground temperature [Kelvin].
 * - ``safetymc``
   - The MacCready setting used, when safety MC is enabled for reach
     calculations, in task abort mode and for determining arrival
     altitude at airfields [:math:`m/s`].
 * - ``riskfactor``
   - The STF risk factor reduces the MacCready setting used to
     calculate speed to fly as the glider gets low, in order to
     compensate for risk.
 * - ``polardegradation``
   - A permanent polar degradation, 1 means no degradation, 0.5
     indicates the glider's sink rate is doubled.
 * - ``arrivalheight``
   - The height above terrain that the glider should arrive at for a
     safe landing [m].
 * - ``terrainheight``
   - The height above trerrain that the glider must clear during final
     glide [m].
 * - ``setmc(value)``
   - Sets the MacCready value [:math:`m/s`].
 * - ``setbugs(value)``
   - Sets the bugs, 1.0 means no bugs, 0.5 means 50% polar degradation.
 * - ``setqnh(float value)``
   - Sets the QNH [Pascal].
 * - ``setballast(float value)``
   - Sets the ballst, 0 means no ballst, 0.5 means 50% of the maximum ballst the glider can carry.
 * - ``setmaxtemp(float value)``
   - Sets the maximum temperature [Kelvin].

.. _lua.wind:

Wind
----

The Settings provides access to XCSoar's wind data and settings.

The following attributes are provided by ``xcsoar.wind``:

.. list-table::
 :widths: 30 70
 :header-rows: 1

 * - Name
   - Description
 * - ``wind_mode``
   - Wind mode, 0: Manual, 1: Circling, 2: ZigZag, 3: Both.
 * - ``setwindmode(int value)``
   - Sets wind mode (0..3).
 * - ``tail_drift``
   - Determines whether the snail trail is drifted with the wind when
     displayed in circling mode, 0: Off, 1: On.
 * - ``settaildrift(bool value)``
   - Turns Taildrift Off / On (0..1).
 * - ``wind_source``
   - The Source of the current wind, 0: None, 1: Manual, 2: Circling,
     3: Both, 4: External.
 * - ``wind_speed``
   - The current wind speed [:math:`m/s`].
 * - ``setwindspeed(float value)``
   - Sets manual the wind speed [:math:`m/s`].
 * - ``wind_bearing``
   - The current wind bearing [°].
 * - ``setwindbearing(float value)``
   - Sets manual the wind bearing [°].
 * - ``clear()``
   - Clears the wind settings and calculations.

.. _lua.logger:

Logger
------

The Settings provides access to XCSoar's Logger data and settings.

The following attributes are provided by ``xcsoar.logger``:

.. list-table::
 :widths: 20 80
 :header-rows: 1

 * - Name
   - Description
 * - ``pilot_name``
   - Gives back the set pilot name.
 * - ``set_pilot_name(name)``
   - Sets the pilot name.
 * - ``time_step_cruise``
   - The time interval between logged points when not circling
     [s].
 * - ``set_time_step_cruise(time)``
   - Sets time interval between logged points when not circling
     [s].
 * - ``time_step_circling``
   - The time interval between logged points when circling [s].
 * - ``set_time_step_circling(time)``
   - Sets time interval between logged points when circling [s].
 * - ``auto_logger``
   - Status of the auto-logger; 0 = On, 1 = Take off only, 2 = Off.
 * - ``set_autologger(mode)``
   - Sets the Autologger mode; 0 = On, 1 = Take off only, 2 = Off.
 * - ``nmea_logger``
   - Status of the NMEA-Logger; 0 = Off, 1 = On.
 * - ``enable_nmea()``
   - Enables the NMEA-Logger.
 * - ``disable_nmea()``
   - Disables the NMEA-Logger.
 * - ``log_book``
   - Status of the log-book; 0 = Off, 1 = On.
 * - ``enable_logbook()``
   - Enables the logbook.
 * - ``disable_logbook()``
   - Disables the logbook.
 * - ``logger_id``
   - The current set logger-id.
 * - ``set_logger_id(id)``
   - Sets the logger ID where ``id`` is a string.

.. _lua.tracking:

Tracking
--------

The Settings provides access to XCSoar's Tracking settings.

The following attributes are provided by ``xcsoar.tracking``:

.. list-table::
 :widths: 40 60
 :header-rows: 1

 * - Name
   - Description
 * - ``skylines_enabled``
   - States if skylines tracking is enabled.
 * - ``enable_skylines()``
   - Enables skylines tracking.
 * - ``disable_skylines()``
   - Disables skylines tracking.
 * - ``skylines_roaming``
   - States if skylines roaming is enabled.
 * - ``skylines_interval``
   - The skylines tracking interval [s].
 * - ``set_skylines_interval(interval)``
   - Sets the tracking interval [s].
 * - ``skylines_traffic_enabled``
   - If enabled, shows friends on the map, download the position of
     your friends live from the SkyLines server.
 * - ``enable_skylines_traffic()``
   - Enables the display of friends on the map.
 * - ``disable_skylines_traffic()``
   - Disables the display of friends on the map.
 * - ``skylines_near_traffic_enabled``
   - If enabled shows nearby traffic
 * - ``enable_skylines_near_traffic()``
   - Enables the display of nearby traffic on the map.
 * - ``disable_skylines_near_traffic()``
   - Disables the display of nearby traffic on the map.
 * - ``livetrack24_enabled``
   - States if livetrack24 is enabled.
 * - ``enable_livetrack24()``
   - Enables livetrack24.
 * - ``disable_livetrack24()``
   - Disables livetrack24.
 * - ``livetrack24_interval``
   - Livetrack24 tracking interval [s].
 * - ``set_livetrack24_interval(interval)``
   - Sets the tracking interval [s].
 * - ``livetrack24_vehicle_name``
   - Get current vehicle name.
 * - ``set_livetrack24_vehiclename(name)``
   - Sets the livetrack24 vehicle name.

.. _lua.replay:

Replay
------

The Settings provides access to XCSoar's Replay system.

The following attributes are provided by ``xcsoar.replay``:

.. list-table::
 :widths: 20 80
 :header-rows: 1

 * - Name
   - Description
 * - ``start(path)``
   - Starts replay from file ``path``.
 * - ``stop()``
   - Stops replay.
 * - ``fast_forward(dt)``
   - Fast forwards ``dt`` [s].
 * - ``set_time_scale(r)``
   - Sets replay clock rate to ``r``.
 * - ``time_scale``
   - Gets replay clock rate.
 * - ``virtual_time``
   - Gets replay virtual time [s].

.. _lua.timer:

Timer
-----

The class ``xcsoar.timer`` implements a timer that calls a given Lua
function periodically.

.. code-block:: lua

 xcsoar.timer.new(60, function(t)
   print("A minute has passed")
   t:cancel()
 end)

The following methods are available in ``xcsoar.timer``:

.. list-table::
 :widths: 40 60
 :header-rows: 1

 * - Name
   - Description
 * - ``new(period, function)``
   - Create a new instance and schedule it.
     ``period`` is a numeric value in seconds.
 * - ``cancel()``
   - Cancel the timer.
 * - ``schedule(period)``
   - Reschedule the timer.

.. _lua.http:

HTTP Client
-----------

The class ``xcsoar.http.Request`` can be used to send HTTP requests.

.. code-block:: lua

  request = xcsoar.http.Request:new(
      "https://download.xcsoar.org/repository")
  response = request:perform()
  print("status", response.status)
  for name, value in pairs(response.headers) do
      print("header", name, ":", value)
  end
  print("body", response.body)

The ``xcsoar.http.Request`` interface:

.. list-table::
 :widths: 20 80
 :header-rows: 1

 * - Name
   - Description
 * - ``new(URL)``
   - Creates a new instance. One instance can only be used once.
 * - ``perform()``
   - Sends the request and waits for the response. Returns a response
     object.

The response interface:

.. list-table::
 :widths: 20 80
 :header-rows: 1

 * - Name
   - Description
 * - ``status``
   - The numeric HTTP status code.
 * - ``headers``
   - A table of response headers.
 * - ``body``
   - The response body (as string).

.. _lua.legacy:

Legacy
------

Before version 7.0, XCSoar was adapted using the *InputEvent*
subsystem (see Appendix `[sec:input-events] <#sec:input-events>`__).
During the Lua transition, Lua scripts can invoke InputEvents, for
example:

.. code-block:: lua

 xcsoar.fire_legacy_event("Setup", "basic")
 xcsoar.fire_legacy_event("Zoom", "basic")
