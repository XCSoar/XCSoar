Weather overlay integration
===========================

This note describes how weather overlays are integrated in the UI and
what invariants new overlay providers must keep.

Scope
-----

Applies to:

- RASP
- EDL
- XCTherm
- SkySight

Code entry points:

- :file:`src/PageActions.cpp`
- :file:`src/Weather/WeatherUIState.hpp`
- :file:`src/Weather/MapOverlay/`
- provider-specific glue in :file:`src/Weather/<Provider>/`

Ownership model
---------------

Weather overlays can be active from two contexts:

- dedicated weather pages (selected through page layout)
- map pages that show the weather cursor bar

The per-overlay :cpp:`OverlaySession` tracks ownership with three flags:

- :cpp:`page_entered`: dedicated page currently owns the overlay
- :cpp:`suspended_for_pan`: dedicated page ownership is temporarily held
  while pan mode is active
- :cpp:`cursor_initialized`: user made an explicit manual selection, so
  auto-reset must not discard it

Use :cpp:`OverlaySession::HasPageOwnership()` when behavior should apply
while either entered or suspended.

Lifecycle in PageActions
------------------------

The lifecycle is orchestrated in :file:`src/PageActions.cpp`:

1. :cpp:`PageActions::ApplyPageOverlay()` selects the overlay for the
   current :cpp:`PageLayout`.
2. Provider-specific apply hooks run:

   - :cpp:`ApplyRaspOverlay(const PageLayout &)`
   - :cpp:`ApplyEdlOverlay()`
   - :cpp:`ApplyXcthermOverlay()`
   - :cpp:`ApplySkySightOverlay()`

3. Leaving a page calls :cpp:`LeaveWeatherOverlayPage()` and
   provider-specific leave hooks.
4. Entering pan mode calls
   :cpp:`SuspendWeatherOverlaysForPan()`.
5. Leaving pan mode calls
   :cpp:`ResumeWeatherOverlaysAfterPan()`.

For dedicated page entry, first-enter behavior should be explicit and
idempotent. Existing providers use :cpp:`EnterPage()` and branch on
``first_enter``.

Per-page cursor state
---------------------

Each weather map page owns both cursor axes. Entering a configured page
restores its selections; Auto values are recalculated from current GPS
time/altitude.

- RASP:

  - Each map page stores its own forecast time in
    :cpp:`PageLayout::rasp_time` (Auto, Now, or a fixed minute-of-day).
  - Entering a RASP page applies that page's field and time via
    :cpp:`Rasp::ApplyTimeFromPageLayout()`.
  - :cpp:`WeatherUIState::ResetRaspForDedicatedPage()` clears the
    effective time only when the page uses Auto
    (:cpp:`time_auto_advance` is true).
  - Time changes from the cursor bar or Weather dialog persist to the
    current page.

- EDL:

  - :cpp:`PageLayout::edl_time` stores Auto, Now, or a fixed UTC
    forecast hour.
  - :cpp:`PageLayout::edl_isobar` stores Auto or a fixed pressure level.
  - page entry applies both values before requesting the overlay.

- XCTherm:

  - :cpp:`PageLayout::xctherm_time` stores Auto or a fixed UTC hour.
  - :cpp:`PageLayout::xctherm_layer` stores Auto or a fixed altitude
    layer.
  - the download Layer, span, cache, and credentials remain global
    provider settings and are not changed by map Altitude selection.

- SkySight:

  - :cpp:`PageLayout::skysight_overlay` stores the selected layer ID.
  - :cpp:`PageLayout::skysight_time` stores Auto or a fixed forecast
    timestamp. Live tile layers always use Auto.
  - entering a SkySight page applies both values. A fixed timestamp is kept
    even when refreshed metadata no longer contains that step, so the cursor
    can show it as unavailable instead of silently changing the page.

SkySight forecast loading
-------------------------

SkySight keeps the selected forecast image visible while it refreshes the
catalog or fetches forecast-step metadata.  With ``Auto update`` enabled,
opening a SkySight page downloads missing or newer data automatically.
Disabling it keeps automatic page refreshes cache-only; explicitly selecting
a forecast time, pressing ``Set active``, or starting a preload can still
download data.  The dialog timer retries deferred metadata requests without
waiting for page activation or map rendering.

``Preload Layer`` and ``Preload Selected`` use the shared top-of-map download
progress widget.  The widget first reports forecast-step discovery, then
displays the number of finished forecast files while download and NetCDF
decoding proceed.  Cached files count as finished.  Requests run one at a
time; a SkySight API rate limit pauses and requeues the interrupted request,
shows a countdown, and resumes automatically.

Preloading includes the available forecasts from the start of the current UTC
day onward and excludes earlier days.  Full-day layers such as PFD contribute
one file per forecast day.  Live tile layers are not preloaded, and their
outstanding requests are cancelled when the user selects another layer.

The SkySight cursor-bar rows are forecast time and layer. Both selections are
stored on the current configured page and restored on page entry. The
SkySight weather panel manages the global selected-layer list, automatic
updates, offline preloading, and cache usage. ``Add to list`` changes the
layers available to every map page. ``Set active`` assigns the highlighted
selected layer to the current page in Auto time mode. More page layouts are
managed through ``Pages setup``.

The cache row reports the total size of the SkySight cache folder. ``Clear
downloaded data`` cancels file downloads and decoding before deleting forecast
files, rendered images, live tiles, and temporary files. Provider catalog
metadata, credentials, selected layers, page configuration, and persisted API
throttle state are preserved.

SkySight overlay rendering
--------------------------

SkySight forecast files hold a NetCDF grid of scalar samples, not a
picture.  :cpp:`SkySightFileDecoder` converts that grid and
:file:`src/Weather/SkySight/FieldImage.cpp` stores it as a single-band
GeoTIFF; :cpp:`SkySightContourOverlay` then contours it for the patch of
map that is on screen.  Provider images (live tiles) keep using the plain
:cpp:`MapOverlayBitmap` path.

Why the field is stored rather than a finished image
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Every SkySight region is continent-sized.  ``EUROPE`` alone is 3002 x 1504
samples at 0.02 degrees, so an image holding one classified pixel per
sample already fills the raster budget.  The map magnifies that texture
with ``GL_LINEAR``, which blends *colours* across a whole grid cell and
produces shades the legend does not contain.  Pre-rendering finely enough
to avoid it is impossible: two pixels per sample would need a 72 MB
texture, and four would exceed the 8192 px axis limit in
:cpp:`LoadTiff()`.

Contouring the visible patch instead keeps the raster small at every
zoom, because a screen only ever shows a few dozen grid cells.

Points to keep in mind when touching this:

- :cpp:`SampleField()` interpolates bicubically (Catmull-Rom) and clamps
  to the four surrounding samples.  Without the clamp, overshoot near
  steep gradients paints bands the forecast never reaches.
- Samples next to missing data fall back to bilinear interpolation over
  the valid neighbours, and become missing once less than half the weight
  is backed by data.  This keeps the coastline of a model domain from
  growing a transparent fringe.
- :cpp:`ContourRasterizer` renders a window but samples the whole field,
  so a patch boundary never becomes a contour boundary.  Keep it that
  way, or panning will show seams.
- :cpp:`GeoBounds` takes north-west and south-east, in that order.
  Swapping them inverts the latitude range and the overlay silently stops
  drawing.
- Samples are packed into 8 bits over the range the data actually covers,
  which keeps a cached forecast smaller than the RGBA image it replaced.
  Widen it only with a measurement: a 16-bit field costs about six times
  the cache.
- :cpp:`SelectWindow()` bounds the raster with
  ``MAX_CONTOUR_RASTER_AXIS`` / ``MAX_CONTOUR_RASTER_CELLS`` from
  :file:`SkySightLimits.hpp`, and targets roughly two screen pixels per
  raster pixel.  Rendering runs in the draw thread, so keep it that way.
- ``FIELD_IMAGE_MAGIC`` identifies the layout.  Change what the decoder
  writes and the magic must change with it, so overlays from an earlier
  decoder are re-fetched rather than drawn as pictures.

Threading and async boundaries
------------------------------

Keep this split:

- UI thread: page transitions, overlay/session state, cursor-bar updates
- network/asio thread: download and parse work
- handoff back to UI: :file:`UI::Notify` callback before UI state or
  overlay updates

Do not call UI APIs (:file:`CommonInterface`, :file:`ActionInterface`,
window code) directly from network worker code.

Integration checklist for a new provider
----------------------------------------

1. Add a provider state/session member in :cpp:`WeatherUIState`.
2. Add dedicated-page apply/leave hooks in :file:`PageActions.cpp`.
3. Handle pan suspend/resume in
   :cpp:`SuspendWeatherOverlaysForPan()` and
   :cpp:`ResumeWeatherOverlaysAfterPan()`.
4. Define first-enter behavior:

   - auto mode reset/refresh path
   - manual mode preserve path via ``cursor_initialized``

5. Trigger refresh/download only from well-defined UI events
   (first enter, auto-no-data, explicit user action).
   Providers with multi-file preloads must report a stable batch progress
   total, not only the current queue depth.
6. Add unit tests for session transitions and reset rules (see
   :file:`test/src/TestWeatherUIState.cpp`).

Common pitfalls
---------------

- clearing manual selections when entering a page
- forgetting to clear ``suspended_for_pan`` on pan exit
- performing UI operations from non-UI threads
- adding provider logic without tests for enter/leave/suspend/resume

Page placement UX
-----------------

Weather dialogs provide a ``Pages setup`` button that opens
Config → Look → Pages, where map overlays (RASP, EDL, XCTherm, SkySight) and the
weather cursor bar can be assigned to pages.

Programmatic placement helpers in ``PageActions`` /
``WeatherMapOverlay::PagePlacement`` still:

- set ``bottom = WEATHER_CONTROLS`` so the shared weather cursor bar is active
- persist page layout changes to the profile
- clear provider cursor initialization so first-enter behavior
  is reapplied consistently

When the page limit (``PageSettings::MAX_PAGES``) is reached, adding a new
page must fail without mutating existing pages.
