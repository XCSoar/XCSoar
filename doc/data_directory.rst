XCSoar data directory
=====================

XCSoar stores user data (profiles, waypoints, logs, downloaded weather,
and similar files) in a **data root** directory. The application refers
to this location as ``%LOCAL_PATH%`` in profile settings.

This chapter describes where that directory lives on each platform and
how files are organised inside it. The layout is implemented by
:file:`Repository/FileType.cpp` (:cpp:func:`GetFileTypeDefaultDir`) and
:file:`DataFileLayout.cpp`.

.. _typed-layout-7-45:

Typed layout (XCSoar 7.45+)
---------------------------

Starting with **XCSoar 7.45**, the typed subdirectory layout described
below is the **standard** for all new and upgraded installations. Earlier
versions stored most files directly in the data root (a “flat” layout).

**Automatic migration:** when you upgrade from an older release, XCSoar
moves recognised files from the data root into the matching subdirectories
on the **first startup** after the upgrade. Profile paths that pointed at
the old locations are updated automatically. No manual file shuffling is
required.

**Migration marker:** after migration completes (or when there is nothing
to move), XCSoar creates an empty marker file
:file:`.xcsoar-subdir-layout-v1` in the data root
(:file:`DataLayoutMigration.cpp`). While this file exists, the migration
is not run again. If every planned move fails (for example because a file
with the same name already exists in the destination subdirectory), the
marker is **not** written and XCSoar will retry on the next startup.

Legacy flat files that were not moved are still found through the
:cpp:func:`ResolveTypedDataFilePath` fallback (see :ref:`profile-paths`
below).

Data root location
------------------

XCSoar may use more than one data root (for example a removable SD card
plus internal storage). The **primary** data root is the first entry in
the search list and is where new files are written.

.. list-table::
   :header-rows: 1
   :widths: 18 32 50

   * - Platform
     - Typical path
     - Notes
   * - Linux / Unix
     - ``~/.xcsoar/``
     - Hidden directory in the user's home folder
   * - Windows
     - ``%LOCALAPPDATA%\XCSoarData\``
     - Also checks ``XCSoarData`` next to :file:`XCSoar.exe` and under
       ``My Documents``
   * - macOS (desktop)
     - ``~/XCSoarData/``
     - Visible folder in the home directory
   * - iOS
     - ``~/Documents/XCSoarData/``
     - Accessible via iTunes / Files when file sharing is enabled
   * - Android
     - App-specific external storage, or legacy
       ``/sdcard/XCSoarData/``
     - Upgrades keep the old location when :file:`xcsoar.log` is found
       there
   * - Kobo
     - ``/mnt/onboard/XCSoarData/``
     - On-board user partition
   * - System-wide (Linux)
     - ``/etc/xcsoar/``
     - Optional global configuration, read after the user directory

Rebranded builds use the product name from :file:`build/product.mk`
instead of ``XCSoar`` / ``XCSoarData``; see :doc:`rebranding`.

Directory layout overview
-------------------------

Since XCSoar 7.45, most user files live in **typed subdirectories**
under the data root (see :ref:`typed-layout-7-45`). Repository downloads,
the file manager, and profile path resolution all use the :cpp:enum:`FileType`
API (:file:`Repository/FileType.hpp`).

.. code-block:: text

   <data-root>/
   ├── airspace/              OpenAir, TNP, SUA files
   ├── cache/                 Repository index, NOTAM cache, weather tiles, …
   │   ├── pc_met/            Flugwetter (PCMet) overlay images
   │   ├── cupx/              Temporary CUPX extraction
   │   └── weather/
   │       ├── edl/mbtiles/   EDL forecast MBTiles cache
   │       ├── rasp/          RASP forecast files (*-rasp*.dat)
   │       └── xctherm/<model>/  XCTherm GeoJSON slice cache
   ├── checklists/            Checklist files (*.xcc, xcsoar-checklist.txt)
   ├── flarm/                 FLARMNet database and FLARM message data
   ├── input/                 Custom input event files (*.xci)
   ├── logs/                  IGC and NMEA flight logs
   ├── lua/                   Lua scripts (*.lua); lua/lib/ for libraries
   ├── maps/                  Terrain files (*.xcm, *.lkm)
   ├── profiles/              Profile files (*.prf)
   │   └── planes/            Plane polar files (*.xcp)
   ├── tasks/                 Saved task files
   ├── waypoints/             Waypoint files (*.cup, *.dat, …)
   │   └── details/           Waypoint detail text files
   ├── weather/
   │   └── overlay/           User-supplied GeoTIFF overlays (*.tif)
   ├── kobo/                  Kobo-specific helper files (Kobo only)
   ├── xcsoar.log             Application log (data root)
   ├── xcsoar-old.log         Rotated log
   ├── xcsoar-startup.log     Early startup log (Android)
   ├── xcsoar-marks.txt       User map markers
   └── .xcsoar-subdir-layout-v1
                              Migration marker (created after layout upgrade)

Typed subdirectories
--------------------

The following table lists each managed subdirectory, the
:cpp:enum:`FileType` value that selects it, and the filename patterns
recognised by :cpp:func:`GetFileTypePatterns`.

.. list-table::
   :header-rows: 1
   :widths: 22 14 64

   * - Subdirectory
     - FileType
     - Filename patterns
   * - :file:`airspace/`
     - ``AIRSPACE``
     - ``*.openair``, ``*.txt``, ``*.air``, ``*.sua``
   * - :file:`waypoints/`
     - ``WAYPOINT``
     - ``*.dat``, ``*.xcw``, ``*.cup``, ``*.cupx``, ``*.wpz``, ``*.wpt``
   * - :file:`waypoints/details/`
     - ``WAYPOINTDETAILS``
     - ``*.txt`` (content sniffing distinguishes waypoint details from
       airspace)
   * - :file:`maps/`
     - ``MAP``
     - ``*.xcm``, ``*.lkm``
   * - :file:`flarm/`
     - ``FLARMNET``, ``FLARMDB``
     - ``*.fln``, ``xcsoar-flarm.txt``, ``flarm-msg-data.csv``
   * - :file:`tasks/`
     - ``TASK``
     - ``*.tsk``, ``*.cup``, ``*.igc``
   * - :file:`checklists/`
     - ``CHECKLIST``
     - ``*.xcc``, ``xcsoar-checklist.txt``
   * - :file:`logs/`
     - ``IGC``, ``NMEA``
     - ``*.igc``, ``*.nmea``
   * - :file:`profiles/`
     - ``PROFILE``
     - ``*.prf`` (including ``default.prf``)
   * - :file:`profiles/planes/`
     - ``PLANE``
     - ``*.xcp``
   * - :file:`input/`
     - ``XCI``
     - ``*.xci``
   * - :file:`lua/`
     - ``LUA``
     - ``*.lua``; put libraries in :file:`lua/lib/` for ``require``
   * - :file:`cache/`
     - (by filename)
     - ``repository``, ``user_repository_*``, ``notams.json``

Ambiguous ``*.txt`` files in the data root are classified by reading
their contents (:file:`DataFileLayout.cpp`). OpenAir and TNP airspace
files go to :file:`airspace/`; waypoint detail files with attachment
sections go to :file:`waypoints/details/`.

.. _cache-directory:

Cache directory
~~~~~~~~~~~~~~~

:file:`cache/` holds data that XCSoar downloads or regenerates and
that should not be edited manually. The whole :file:`cache/` tree is
**excluded from data backups** (:file:`Dialogs/DataManagement/BackupRestorePanel.cpp`).

- ``repository`` — main file repository index
- ``user_repository_<name>`` — additional user-defined repository files
- ``notams.json`` — cached NOTAM data
- ``pc_met/`` — downloaded Flugwetter (PCMet) overlay images
- ``cupx/`` — temporary files while opening CUPX waypoints
- ``weather/rasp/`` — RASP forecast files (``*-rasp*.dat``, for example
  :file:`xcsoar-rasp.dat`)
- ``weather/edl/mbtiles/`` — downloaded EDL forecast MBTiles tiles
- ``weather/xctherm/<model>/`` — XCTherm GeoJSON slice cache files

Downloaded weather data (RASP, EDL, XCTherm) is stored here rather than
under the top-level :file:`weather/` directory, so it is excluded from
backups together with other cache data. Earlier 7.45 development builds
used :file:`weather/rasp/`, :file:`weather/edl/`, and
:file:`weather/xctherm/` directly in the data root; those paths may
still be read for backwards compatibility until the cache move is
complete.

Legacy copies of ``repository`` and ``notams.json`` in the data root
or in a top-level :file:`repository/` folder are still resolved for
backwards compatibility (:file:`DataFilePath.cpp`).

Weather data
~~~~~~~~~~~~

The top-level :file:`weather/` directory is for **user-provided**
overlay images only:

- **GeoTIFF overlays** — :file:`weather/overlay/` (``*.tif``, ``*.tiff``), listed
  in the Weather dialog
  (:file:`Dialogs/Weather/MapOverlayWidget.cpp`).

Repository downloads and other fetched forecast data live under
:file:`cache/weather/` (see :ref:`cache-directory`):

- **RASP** — :file:`cache/weather/rasp/` (:file:`Weather/Rasp/`). The
  default filename is :file:`xcsoar-rasp.dat`.
- **EDL** — :file:`cache/weather/edl/mbtiles/`
  (:file:`Weather/EDL/TileStore.cpp`). Filenames encode forecast time
  and isobar level.
- **XCTherm** — :file:`cache/weather/xctherm/<model>/`
  (:file:`Weather/xctherm/XCThermAPI.cpp`). Each file is named
  ``<parameter>_<utc>.xctcache``.

Files in the data root
----------------------

Some paths intentionally remain at the top level of the data root:

- **Log files** — :file:`xcsoar.log`, :file:`xcsoar-old.log`,
  :file:`xcsoar-startup.log`, :file:`xcsoar-persist.log`
  (:file:`LogFile.cpp`)
- **Markers** — :file:`xcsoar-marks.txt` (:file:`Markers/Markers.cpp`)
- **Migration marker** — :file:`.xcsoar-subdir-layout-v1` — empty file
  written after the 7.45 layout migration; see :ref:`typed-layout-7-45`

Kobo builds additionally use :file:`kobo/` for power-off text, Wi-Fi
auto-on flags, OTG host state, and user scripts.

.. _profile-paths:

Profile paths
-------------

Profile settings store file references as ``%LOCAL_PATH%\<relative-path>``
(:file:`LocalPath.cpp`). After migration, paths point at the typed
subdirectories (for example
``%LOCAL_PATH%\waypoints\waypoints.cup``).

:cpp:func:`ResolveTypedDataFilePath` and :cpp:func:`ResolveLocalDataFile`
(:file:`DataFilePath.cpp`) look up files in the typed subdirectory first,
then fall back to the data root for legacy flat layouts.

Upgrading from a flat layout
----------------------------

This section summarises the automatic migration introduced in 7.45; see
:ref:`typed-layout-7-45` for the marker file and when migration runs.

On first start after upgrading to the typed layout, XCSoar scans the
data root and **moves** recognised files into the matching
subdirectories (:file:`DataLayoutMigration.cpp`). A file is moved only
when:

- its type can be determined from the filename (or content, for
  ambiguous ``*.txt`` files), and
- no file with the same name already exists at the destination.

After at least one successful move, XCSoar writes
:file:`.xcsoar-subdir-layout-v1` and updates profile references to the
new paths. If there was nothing to migrate, the marker is still created
so the scan is not repeated on every startup. Files that cannot be
classified (or would overwrite an existing destination file) are left in
the data root.

The file manager and data-management tools understand the typed layout:
downloads are stored in the correct subdirectory, and files can be
transferred between storage volumes with paths rewritten automatically.

Related documentation
---------------------

- :doc:`lua` — Lua script location under :file:`lua/`
- :doc:`checklist` — checklist file format and search order
- :doc:`rebranding` — platform-specific data directory names
- :doc:`architecture` — :file:`Repository/` and background downloads
