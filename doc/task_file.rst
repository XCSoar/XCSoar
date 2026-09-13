Task file format (.tsk)
=======================

XCSoar stores an ordered task in a file with the extension ``.tsk``.  It
is a small XML document with a ``<Task>`` root element.  One file holds
exactly one task.

This page describes the format well enough to write a converter, a
WeGlide-style exporter, or a declaration path that agrees with what
XCSoar itself writes.

The normative definition is the round trip between
``src/Task/Serialiser.cpp`` (writer) and ``src/Task/Deserialiser.cpp``
(reader); the file wrapper lives in ``src/Task/SaveFile.cpp`` and
``src/Task/LoadFile.cpp``.  When this page and the code disagree, the
code wins -- please fix the page.

There is deliberately **no XSD**.  The format is whatever the serialiser
writes, and a schema would rot the first time an attribute is added.

Document structure
------------------

The shape of the document is::

   <Task type="RT" aat_min_time="10800" start_requires_arm="0" ...>
           <Point type="Start">
                   <Waypoint name="Wanlo Niersq" id="3675" comment="121.175 0826" altitude="74">
                           <Location longitude="6.39361" latitude="51.1011"/>
                   </Waypoint>
                   <ObservationZone type="Cylinder" radius="1000"/>
           </Point>
           ...
   </Task>

Notes on the wrapper:

-  XCSoar writes no ``<?xml ... ?>`` declaration.  The parser skips one
   if a file has it.

-  Text is UTF-8, and no encoding is declared or negotiated -- strings
   are passed through as bytes.

-  Elements are indented with tabs; whitespace is not significant.

-  The writer escapes ``<``, ``>``, ``&``, ``'`` and ``"``.  The reader
   understands exactly those five entities plus **decimal** character
   references (``&#160;``).  Hexadecimal references (``&#xA0;``) and any
   other entity make the parse fail, and the whole file is rejected.

-  The parser refuses files larger than **65536 bytes**
   (``src/XML/Parser.cpp``).

-  Element and attribute *names* are matched case-insensitively.
   Attribute *values* that act as enumerations (``type``, the height
   references) are compared case-sensitively -- write ``Cylinder``, not
   ``cylinder``.  A misspelt zone type is not an error: the point simply
   gets the factory's default zone.

-  Attribute order carries no meaning, and XCSoar's writer does not
   preserve it -- in current output ``type`` comes last on ``<Task>``.
   The examples here are wrapped across lines for readability; XCSoar
   writes each start tag on one line.

``<Task>``
----------

The root element carries the task type and the start/finish
constraints.  XCSoar writes every attribute below on every save, except
the two start gate times, which are omitted when no gate is set.

``type``
   Selects the task factory, which in turn decides which point and zone
   types are legal and what the task is scored as.  One of
   ``FAIGeneral``, ``FAITriangle``, ``FAIOR``, ``FAIGoal``, ``RT``
   (racing), ``AAT``, ``MAT``, ``Mixed``, ``Touring``.  A missing or
   unrecognised value falls back to ``FAIGeneral``.

``aat_min_time``
   Minimum task time in **seconds**, as an integer.  Only meaningful for
   ``AAT`` and ``MAT``.

``navigate_nearest``
   ``0`` or ``1``.  Whether navigation to the start and to the finish
   aims at the nearest point of their zone rather than at the point
   which makes the task shortest.  Lines and cylinders honour it;
   sectors and keyholes ignore it, and so does a cylinder the glider is
   already inside.  Scoring and the planned task distance are not
   affected.

``start_requires_arm``
   ``0`` or ``1``.  Whether the start has to be armed manually.

``start_score_exit``
   ``0`` or ``1``.  Whether leaving the start zone scores the start
   (``1``) rather than entering it.

``start_max_speed``
   Maximum ground speed in the start zone, in **m/s**.  ``0`` means no
   limit.

``start_max_height``
   Maximum height in the start zone, in **metres**, as an integer.
   ``0`` means no limit.

``start_max_height_ref``
   ``MSL`` or ``AGL``.  Any value other than the exact string ``MSL`` is
   read as ``AGL`` -- which is how the numeric values written by very old
   versions still load.

``start_open_time``, ``start_close_time``
   The hard start gate, as ``HH:MM`` in **UTC**, on a 24-hour clock.
   Either is omitted when that end of the gate is not set, which leaves
   the gate open-ended on that side; a value that does not parse as
   ``HH:MM`` is treated the same way.  Unlike every other attribute
   here, these two are read unconditionally: omitting both *clears* the
   start gate rather than leaving the default in place.

``pev_start_wait_time``
   Seconds to wait after a Pilot Event before the start gate opens.

``pev_start_window``
   Seconds the start gate stays open after the PEV wait time has
   elapsed.

``finish_min_height``
   Minimum finish height in **metres**, as an integer.  ``0`` means no
   limit.

``finish_min_height_ref``
   ``MSL`` or ``AGL``, read the same way as
   ``start_max_height_ref``.

``fai_finish``
   ``0`` or ``1``.  When set, FAI start and finish height rules apply and
   the start/finish constraints above are ignored.

``<Point>``
-----------

``<Task>`` contains one ``<Point>`` per task point, in task order,
followed by any optional start points.  Each ``<Point>`` has a ``type``
attribute:

``Start``
   The start point.
``Turn``
   An AST turn point -- the aircraft must reach the zone.
``Area``
   An AAT area -- the aircraft may turn anywhere inside the zone.
``Finish``
   The finish point.
``OptionalStart``
   An alternative start.  These are appended to the task's optional
   start list rather than to the task itself, and XCSoar writes them
   after all ordinary points.

A ``<Point>`` with no ``type``, with no ``<Waypoint>`` child, or with a
``<Waypoint>`` that has no ``name`` or no ``<Location>``, is skipped
silently.

``Turn`` points additionally accept ``score_exit="1"``, meaning that
leaving the zone scores the turn.  XCSoar writes the attribute only when
it is set.

``<Waypoint>`` and ``<Location>``
---------------------------------

``<Waypoint>`` names the point and carries a mandatory ``<Location>``
child::

   <Waypoint name="Weisweiler K" id="3649" comment="Kw 1011Ft" altitude="144">
           <Location longitude="6.32278" latitude="50.8397"/>
   </Waypoint>

``name``
   Required.  A point whose waypoint has no name is skipped.

``id``
   Numeric waypoint id, for matching against a waypoint database.
   Optional.

``comment``
   Free text.  Optional.

``altitude``
   Waypoint elevation in **metres**.  Optional; when absent, the
   waypoint is loaded without a known elevation.

``<Location>`` carries ``longitude`` and ``latitude`` as **decimal
degrees**, east and north positive.

XCSoar always writes ``id`` and ``comment``, falling back to ``0`` and
the empty string; ``altitude`` is written only for a waypoint with a
known elevation.

When a waypoint database is available, the reader first looks up
``name`` in it and uses the database entry if it lies within **10 m** of
``<Location>``; failing that it takes the nearest database waypoint
within 10 m.  Only if neither matches is a waypoint built from the file.
This means ``id``, ``comment`` and ``altitude`` in the file are hints --
they are dropped whenever the database wins.

``<ObservationZone>``
---------------------

Each ``<Point>`` should carry one ``<ObservationZone>`` child.  Its
``type`` selects the shape; the remaining attributes depend on the
shape.  A point whose zone is missing or whose ``type`` is unknown still
loads, but gets the factory's default zone for that position.

Radii and lengths are in **metres**, angles in **degrees**.

.. list-table::
   :header-rows: 1
   :widths: 22 30 48

   * - ``type``
     - Attributes
     - Notes
   * - ``Line``
     - ``length``
     - A line, normally used for start and finish.  ``length`` is the
       full width; default 1000.
   * - ``Cylinder``
     - ``radius``
     - Default 10000.
   * - ``MatCylinder``
     - --
     - Fixed 1 statute mile (1609.344 m) MAT cylinder.
   * - ``Sector``
     - ``radius``, ``start_radial``, ``end_radial``,
       optionally ``inner_radius``
     - A sector at fixed radials (true bearings).  Adding
       ``inner_radius`` turns it into an annular sector.  Defaults:
       radius 10000, radials 0 and 360.
   * - ``FAISector``
     - --
     - The 90 degree FAI quadrant on the leg bisector: 10000 m at a turn
       point, 1000 m at start/finish.
   * - ``SymmetricQuadrant``
     - ``radius``, ``angle``
     - A sector of ``angle`` centred on the leg bisector.  Despite the
       name it need not be a quadrant -- any angle works.  Defaults:
       radius 10000, angle 90.
   * - ``Keyhole``
     - --
     - DAeC keyhole: 90 degree, 10000 m sector plus a 500 m cylinder.
   * - ``CustomKeyhole``
     - ``radius``, ``angle``, ``inner_radius``
     - Keyhole with a configurable sector and inner cylinder.  Defaults:
       radius 10000, angle 90, inner radius 500.
   * - ``BGAStartSector``
     - --
     - BGA start: 180 degree, 5000 m sector.
   * - ``BGAFixedCourse``
     - --
     - BGA fixed course keyhole: 90 degree, 20000 m sector plus a 500 m
       cylinder.
   * - ``BGAEnhancedOption``
     - --
     - BGA enhanced option keyhole: 180 degree, 10000 m sector plus a
       500 m cylinder.

The zone types that take no attributes are fully described by their
name; XCSoar writes nothing else for them and ignores anything else it
finds.

Load rules
----------

These are the rules a converter has to know:

-  **Unknown attributes and unknown elements are ignored.**  The reader
   only ever asks for the attributes it knows.  Files still in the wild
   carry attributes that XCSoar stopped writing years ago
   (``task_scored``, ``min_points``, ``max_points``, ``homogeneous_tps``,
   ``is_closed``); they load fine and are dropped on the next save.

-  **Missing attributes are not "the default".**  They keep whatever a
   freshly created task has, which is the pilot's configured task
   defaults (Config > Task > Rules and Defaults), narrowed by the
   factory for ``type``.  Two pilots opening the same file can therefore
   get different start heights.  A converter that cares about a
   constraint must write it explicitly.

-  **The factory has the last word on point and zone types.**  Once the
   points are read they are normalised to what the ``type`` factory
   allows.  A point type the factory does not use is swapped (``Turn``
   in an ``AAT`` task comes back as ``Area``), and a zone type it does
   not permit is replaced by its own substitute -- in an ``AAT`` a
   ``SymmetricQuadrant`` turn becomes a ``Cylinder``, in an
   ``FAITriangle`` it becomes an ``FAISector``.

-  **``FAIGeneral`` and ``MAT`` rebuild every point.**  Both rebuild all
   points at the factory's standard dimensions rather than only the
   invalid ones, so the radii and lengths in the file are discarded.  A
   5000 m start line loaded as ``FAIGeneral`` comes back as the FAI
   1000 m line, and a 1000 m turn cylinder as the FAI 500 m one.
   ``Touring`` loses nearly as much, because it accepts so few zone
   types.

-  **Write** ``RT`` **when the geometry has to survive.**  It preserves
   every zone type in the table above, with its dimensions, except the
   AAT-only shapes -- and ``AAT`` does the same for the area shapes.

-  **Booleans are integers.**  ``1`` for true; anything that parses to
   zero or less is false.  Do not write ``true``/``false``.

-  **Numbers are C locale.**  Decimal point, no thousands separator.
   XCSoar writes doubles with ``%g``, so ``6.39361`` and ``1e+06`` are
   both possible on output.

-  A file whose root element is not ``<Task>`` is rejected outright.

What this format does *not* carry
---------------------------------

Two neighbouring formats have zone fields that look like they belong
here.  They do not, and there is nothing in ``.tsk`` to map them onto:

-  SeeYou ``.cup`` ``NearDis`` and ``NearAlt`` are *scoring tolerances*
   for how close to a turnpoint still counts.  XCSoar does not read them
   and has no equivalent -- an observation zone is either reached or not.

-  The LXNAV ``LLXVTSK`` / ``LLXVOZ`` declaration lines carry ``Near=``,
   which tells the *logger* to switch to the next point automatically.
   That is a property of the declaration XCSoar sends to the device
   (``src/Device/Driver/LX/LXNavDeclare.hpp``), derived at declaration
   time, not something read back from or stored in the task file.

Keep both out of converters.  What XCSoar itself aims at is
``navigate_nearest`` above; a ``.tsk`` that grew a ``near`` attribute
would be silently ignored and misleading to everyone else.

A complete example
------------------

A racing task with a start line, two turn points and an FAI finish
sector, as XCSoar writes it::

   <Task pev_start_window="0" pev_start_wait_time="0" fai_finish="0"
         finish_min_height_ref="AGL" finish_min_height="0"
         start_close_time="16:00" start_open_time="11:30"
         start_max_height_ref="AGL" start_max_height="0" start_max_speed="0"
         start_score_exit="1" start_requires_arm="0"
         navigate_nearest="0" aat_min_time="10800" type="RT">
   	<Point type="Start">
   		<Waypoint altitude="187" comment="" id="1" name="Aachen Merzbrueck">
   			<Location latitude="50.8231" longitude="6.18639"/>
   		</Waypoint>
   		<ObservationZone length="5000" type="Line"/>
   	</Point>
   	<Point type="Turn">
   		<Waypoint altitude="144" comment="" id="3649" name="Weisweiler K">
   			<Location latitude="50.8397" longitude="6.32278"/>
   		</Waypoint>
   		<ObservationZone radius="1000" type="Cylinder"/>
   	</Point>
   	<Point type="Turn">
   		<Waypoint altitude="86" comment="" id="3883" name="Langenfeld W">
   			<Location latitude="51.1408" longitude="6.98528"/>
   		</Waypoint>
   		<ObservationZone angle="90" radius="10000" type="SymmetricQuadrant"/>
   	</Point>
   	<Point type="Finish">
   		<Waypoint altitude="187" comment="" id="1" name="Aachen Merzbrueck">
   			<Location latitude="50.8231" longitude="6.18639"/>
   		</Waypoint>
   		<ObservationZone type="FAISector"/>
   	</Point>
   </Task>

``test/data/apf-bug554.tsk`` is a real file in the tree, still carrying
the obsolete attributes mentioned above.

Testing a converter
-------------------

``DumpTaskFile`` loads a task file and writes the task back out in this
format, which makes the round trip directly visible::

   ./output/UNIX/bin/DumpTaskFile test/data/apf-bug554.tsk 0

The trailing ``0`` is the task index; without it the tool only lists the
tasks a file contains.  Diffing its output against the input is the
quickest way to see what XCSoar actually understood -- everything it
dropped, defaulted or mutated shows up there.

``TaskInfo FILE.tsk ...`` prints the resulting task geometry and
statistics instead.

Both are debug utilities, built by ``make debug`` rather than by plain
``make``; see :doc:`test_debug_utilities`.
