============
Architecture
============

This chapter describes XCSoar’s internal code architecture.

Source Organisation
-------------------

XCSoar’s source code is stored in the :file:`src` directory. This
section tries to give a rough overview where you can find what.

-  :file:`util/`: generic C++ utilities that do not depend on external
   libraries, such as data structures, string operations

-  :file:`Math/`: math data types (fixed-point math, angles) and generic
   formulas

-  :file:`Geo/`: geographic data structures and formulas

-  :file:`Formatter/`: code that formats internal values to strings

-  :file:`Units/`: conversion from SI units (“System” units) to configured
   user units

-  :FILE:`NMEA/`: data structures for values parsed from NMEA

-  :file:`Profile/`: user profiles, loading from and saving to

-  :FILE:`IGC/`: support for the IGC file format

-  :file:`Logger/`: all loggers (NMEA, IGC, flights)

-  :file:`thread/`: multi-threading support (OS specific)

-  :file:`ui/`: base library for the graphical user interface

-  :file:`Renderer/`: various graphical renderers, for map and analysis

-  :file:`MapWindow/`: the map

-  :file:`Form/`: modal dialogs and their controls (based on the screen
   library)

-  :file:`Dialogs/`: modal dialogs implementations (based on the form
   library)

-  :file:`net/`: networking code (OS specific)

-  :file:`Operation/`: generic code to support cancellable long-running
   operations

-  :file:`Android/`: code specific to Android (the native part only; Java
   code is in :file:`android/src/`

-  :file:`Engine/PathSolvers/`: an implementation of Dijkstra’s path finding
   algorithm, for task and contest optimisation

-  :file:`Engine/Airspace/`: airspace data structures and airspace warnings

-  :file:`Engine/Waypoint/`: waypoint data structures

-  :file:`Engine/GlideSolvers/`: a MacCready implementation

-  :file:`Engine/Task/`: task data structures and calculations

-  :file:`Engine/Contest/`: contest optimisation

-  :file:`Engine/Route/`: the route planner (airspace and terrain)

Threads and Locking
-------------------

Threads
~~~~~~~

XCSoar runs on multiple threads, to make the UI responsive but still
allow expensive background calculations.

This is how it looks like on Windows and Linux/SDL (software rendering):

.. blockdiag::

    blockdiag threads {
      Devices [stacked];
      Devices -> MergeThread [label = "sensors", fontsize=8];
      MergeThread -> CalcThread [label = "sensors", fontsize=8];

      CalcThread -> UIThread [folded, label = "results", fontsize=8];

      IOThread -> UIThread [label = "data", fontsize=8];

      UIThread <-> DrawThread [label = "redraw", fontsize=8];
    }

The UI thread is the main thread.  It starts the other threads and is
responsible for the UI event loop.  No other thread is allowed to
manipulate windows.  The UI thread has a timer which does regular house
keeping twice per second (:file:`ProcessTimer.cpp`).

The calculation thread (:file:`CalculationThread.cpp`,
:file:`GlideComputer*.cpp`) does all the expensive calculations in
background.  It gets data from the devices (through :file:`MergeThread`) and
forwards it together with calculation results to the drawing thread and
the main thread.

Each device has its own thread (:file:`SerialPort.cpp`).  This is
needed because Windows CE does not support asynchronous COMM port
I/O. The thread is stopped during task declaration (which happens in
the UI thread).

When new data arrives on the serial port, the :file:`MergeThread` gets
notified, which will merge all sensor values into one data structure. It
will then run cheap calculations, and forwards everything to the
:file:`CalculationThread`.

With OpenGL, the map is rendered live without a buffer. There is no
DrawThread.

On Android, the UI thread is not the main thread - the main thread is
implemented in Java, managed by Android itself. The UI thread listens
for events which the Java part drops into the event queue
(:file:`NativeView.java` and others). The internal GPS does not need a
thread, it is implemented with Java callbacks. For Bluetooth I/O, there
are two threads implemented in Java (:file:`InputThread.java` and
:file:`OutputThread.java`, managed by :file:`BluetoothHelper.java`).

Locking
~~~~~~~

Some data structures are rarely modified. There is no lock for them. For
a modifications, all threads must be suspended. Example: waypoints,
airspaces.

Other data structures are modified so often that correct locking would
be too much overhead. Each thread and each instance has its own copy.
The lock needs to be obtained only for making the private copy. The
private copy can be used without locking. Example: ``NMEA_INFO``,
``DERIVED_INFO``.

There are objects which are too expensive to copy. Normal locking
applies to them. We have a template class called ``Guard`` to enforce
proper read/write locking. Example: the task.

Accessing Sensor Data
---------------------

Much of XCSoar deals with obtaining sensor data and visualising it.

Suppose you want to write a dialog that needs the current GPS location,
where do you get it? The short and simple answer is: from
``CommonInterface::Basic()`` (the ``InterfaceBlackboard``). Example::

   #include "Interface.hpp"

   ...
     const auto &basic = CommonInterface::Basic();
     if (basic.location_available)
       current_location = basic.location;

This is true for the main thread (aka the “user interface thread”).
Other threads must not use the :file:`Interface.hpp` library, because the
``InterfaceBlackboard`` is not protected in any way. It contains copies
of various data structures just for the main thread.

This is how sensor data moves inside XCSoar:

.. blockdiag::

    blockdiag threads {
      Devices [stacked];
      MergeThread [label="MergeThread\nBasicComputer\nDeviceBlackboard"];
      CalcThread [label="CalcThread\nGlideComputer\nGlideComputerBlackboard"];
      UIThread [label="UIThread\nInterfaceBlackboard\nBlackboardListener"];
      DrawThread [label="DrawThread\nMapWindow\nMapWindowBlackboard"];

      Devices -> MergeThread [folded, label = "NMEAInfo", fontsize=8];
      MergeThread -> CalcThread [folded, label = "MoreData", fontsize=8];

      CalcThread -> UIThread [folded, label = "DerivedInfo", fontsize=8];
      UIThread -> DrawThread [folded, label = "DerivedInfo", fontsize=8];
    }

The device driver parses input received from its device into its own
``NMEAInfo`` instance inside ``DeviceBlackboard`` (i.e.
``per_device_data``). Then it wakes up the ``MergeThread`` to merge the
new data into the central ``NMEAInfo`` instance. The ``MergeThread``
hosts the ``BasicComputer`` which attempts to calculate missing data
(for example, derives vario from GPS altitude).

The ``CalculationThread`` wakes up and receives the ``MoreData`` object
from ``DeviceBlackboard``. Here, expensive calculations are performed
(``GlideComputer``: task engine, airspace warnings, ...), resulting in a
``DerivedInfo`` object. The ``CalculationThread`` runs no more than
twice per second.

Finally, the UI thread wakes up and receives ``MoreData`` and
``DerivedInfo`` via ``DeviceBlackboard``. This updates InfoBoxes and
other UI elements. On Windows, the map is drawn in a separate thread, so
there’s another layer.

Let’s get back to the question: where do I get sensor data? That depends
on who you are:

- you are the user interface: (InfoBoxes, dialogs, any Window
  callback): ``InterfaceBlackboard`` (see above). To get notified on
  changes, register a ``BlackboardListener`` (and don’t forget to
  unregister it).

- you are the MapWindow: depends! If you’re being called from
  ``OnPaintBuffer`` (i.e. inside the ``DrawThread``), you must use the
  ``MapWindowBlackboard``, all others must use the
  ``InterfaceBlackboard``.

- you are a “computer” library: you will get the values as a parameter.
  Don’t try to use the ``GlideComputerBlackboard`` directly.

- you are a device driver: implement the method ``OnSensorUpdate`` or
  ``OnCalculatedUpdate`` if you need to know values from other devices
  or calculation results.

- everybody else may use the ``DeviceBlackboard``, but be sure to lock
  it while using its data.

Developing
==========

Debugging XCSoar
----------------

The XCSoar source repository contains a module for the GNU debugger
(``gdb``). It contains pretty-printers for various XCSoar types,
including ``Angle``, ``GeoPoint`` and others. These are helpful when you
print values in the debugger. To use it, start the debugging session and
load the module::

  $ gdb -ex "source tools/gdb.py" output/UNIX/bin/xcsoar
  (gdb) run

The module will automatically convert fixed-point to floating point,
radian angles to degrees and more. You can now do fancy stuff like::

  (gdb) p basic.location
  $1 = GeoPoint(7.93911242887 51.1470221074)
  (gdb) p basic.date_time_utc
  $2 = DateTime(2012/12/23 21:41:57)
  (gdb) p basic.track
  $3 = 55.2254197961
  (gdb) p basic.external_wind
  $4 = GeoVector::ZERO
  (gdb) p current_leg.vector_remaining
  $5 = GeoVector(267.899420345 107957.109724)

User interface guidelines
=========================

General
-------

-  Minimise the number of colours, and re-use colour groups already
   defined.

-  Too much use of colour where it is not required serves only to reduce
   the effectiveness of bright colours for important items.

-  High colour saturation elements should be reserved for high
   importance items

-  High contrast against background should be reserved for high
   importance items

-  Attempt to adopt colours that are intuitive based the function of the
   item

-  Minimise the clutter where possible — readibility is essential for
   use in flight

-  Use colours defined in ``Graphics`` according to functional name, not
   their actual colour.

-  Try to maintain consistent use of colours in all uses of that
   function, such as dialogue graphics as well as map overlays and
   infoboxes.

-  Text should always be monochrome.

Use aviation conventions or adopt best aviation human factors standards
where possible, in particular:

-  ICAO Internation Standards and Recommended Practices, Annex 4 to the
   Convention on International Civil Aviation (Aeronautical Charts).

- `NASA Colour Usage recommendations and design guidelines
   <http://colorusage.arc.nasa.gov/>`__

- `DOT/FAA/AR-03/67 Human Factors Considerations in the Design and
   Evaluation of Electronic Flight Bags (EFBs)
   <http://www.volpe.dot.gov/hf/aviation/efb/docs/efb_version2.pdf>`__

-  `FAA Human Factors Design Standards <http://hf.tc.faa.gov/hfds/>`__

-  DOT/FAA/AM-01/17 Human Factors Design Guidelines for Multifunction
   Displays

Check for performance with respect to colour blindness.  This site has
a useful tool that can be used to convert screenshots to how they
would look to a person with common color blindness:
http://www.etre.com/tools/colourcheck/

**For safety purposes, avoid use of elements that may encourage or
require the user to stare at the screen continuously.**

**For safety purposes, avoid user controls that have significant risk
of producing unsafe results if misconfigured by the pilot.**

General colour conventions
~~~~~~~~~~~~~~~~~~~~~~~~~~

Colour conventions generally in use throughout the program:

-  Red for indicator of warning

-  Orange for indicator of caution

-  Green for positive indicator of safety

-  Blue for neutral indicator of safety

Displayed data
~~~~~~~~~~~~~~

-  Where data is invalid, indicate this by not presenting the data or
   showing dashes.

-  Present data in user-defined units.

-  Display numerical data with significant digits appropriate to the
   accuracy of the calculations, or its functional use by the pilot,
   whichever is lower.

Dialogs and menu buttons
------------------------

Colors
~~~~~~

Colour conventions in use are:

-  Grey for buttons

-  Buttons and other widgets rendered with an evenly shaded border

-  Yellow for clicked items

-  Light blue for the key focused item

-  Medium blue for dialogue title bar

-  Text is black if the item is enabled

-  Text is greyed out (but still visible) if the item is disabled

dialogue types and navigation buttons
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

There are four types of dialogs in XCSoar, and the navigation buttons
for each are different. Navigation buttons are the Close, OK, Cancel and
Select buttons.

-  Dialogs that modify and save data when the dialogue closes.

   These shall usually have a Close button (no Cancel) and may have
   context specific function buttons

-  Dialogs that modify data where Cancel would be important for the
   user.

   These shall have OK and Cancel buttons. This may include dialogs with
   children dialogs where hitting Cancel from the parent dialogue
   cancels all the changes made in the children dialogs

-  Dialogs that have a list of values, one of which can be selected to
   return to the parent dialogue.

   These shall have Select and Cancel buttons

-  Dialogs that display information that cannot be modified.

   These shall have a Close button

dialogue button placement and size
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

-  The Close and Cancel buttons will never appear in the same dialogue
   and are always located in the same place. This location will be:

   For portrait: lower right

   For landscape: lower left

-  The Select button will be accompanied with a Cancel button. The
   locations will be:

   For portrait: Select in lower left, Cancel in lower right

   For landscape: Cancel in lower left, Select immediately above it

-  Buttons will be 35 (scaled) pixels high

-  Buttons will be flush with the bottom of the screen and with the
   sides of the screen and against each other (no margins)

-  In portrait, buttons will be 33

-  In landscape, buttons will be 65 to 80 (scaled) pixels wide, as wide
   as the frame permits. They will generally be a vertical row of
   buttons flush left of the screen

-  If text won’t fit on a button, the buttons can be made larger
   consistently for a screen, but this should be the exception because
   if it must contain that much text consider using a different type of
   control.

-  Exceptions to all the dialogue concepts above are encouraged, but
   should be mocked up and reviewed with the development community prior
   to implementing and possibly documenting in the developers guide.

Usability
~~~~~~~~~

-  Minimum size of buttons should be X by Y mm

-  Ensure all dialogs are navigable using cursor keys only

-  Ensure the focussed item is clearly identified. The rectangle of the
   widget on the canvas may be drawn using the ``fill_focus`` method of
   ``Canvas``.

Main graphics
-------------

Colors
~~~~~~

Colour conventions in use, in order of priority, are:

-  Aircraft black and white, for neutrality but clear identification

-  Traffic (FLARM) use alarm green, orange, and red.

-  Lift is vibrant green, sink is copper orange.

-  Aircraft navigation (route, best cruise track) is (ICAO) dark
   purple-blue

-  Task navigation lines and areas are (ICAO) magenta.

-  Updraft sources and other updraft derived data is sky blue.

(Todo) airspace alert colours

Map culture (topography) and terrain rendering should conform to ICAO
Annex 4 where appropriate. Note that some modifications are reasonable
for electronic use given that Annex 4 deals with paper charts.
Nevertheless, the colour conventions are useful to adopt as they are
likely to be intuitive and are designed for aviation use.

Pen styles
~~~~~~~~~~

-  Map culture should be rendered with a thin pen

-  Thicker pens used for important (e.g. task, navigational, airspace)
   lines

-  Dashed lines are used to increase perceptual priority

Map overlays
~~~~~~~~~~~~

Elements on the map that are not part of the map layer, such as
additional informational widgets (final glide bar, wind, north arrow)
should be rendered so as to help those elements be visually separated
from the map:

-  Generally adopt higher contrast (higher colour saturation or darker
   shade) than the background map layer elements.

-  For elements covering an area (non line), draw the entire element or
   a border with a luminosity contrasting pen, of width ``IBLSCALE(1)``.

-  Consider whether the widget is required in all flying states and
   display modes. if it does not serve a direct functional purpose in
   some states/modes, do not render it.

-  Avoid locating widgets at the aircraft symbol (ownship symbol). It is
   important to keep this area clear so the aircraft symbol can be
   easily found.

Elements that may be rendered over each other should be organised in
order of priority, particularly with alert warning items above caution
items above non-alert items.

Terminology
-----------

Glide Ratio
~~~~~~~~~~~

’Glide ratio’ is a non-specific term which can refer to the ratio of
horizontal to vertical motion with reference to either the surrounding
airmass or the ground.

To reduce confusion, ground-referenced glide ratios (eg distance
travelled over ground vs altitude lost) should be referred to by the
term ’glide ratio over ground’ when space allows, or ’glide ratio’ /
’GR’.

Air-referenced glide ratios (eg airspeed vs sink rate) should be
specified as ’lift/drag ratio’ / ’L/D ratio’ / ’LD’. The lift/drag ratio
is numerically equal to the air-referenced glide ratio when flying at
constant speed.

If usage spans both air-referenced and ground-referenced glide ratios,
the non-specific term ’glide ratio’ / ’GR’ should be used. ’Lift/drag
ratio’ should never be used to refer to ground-referenced glide ratios.
