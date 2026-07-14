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
   - :cpp:`ApplySkysightOverlay()`

3. Leaving a page calls :cpp:`LeaveWeatherOverlayPage()` and
   provider-specific leave hooks.
4. Entering pan mode calls
   :cpp:`SuspendWeatherOverlaysForPan()`.
5. Leaving pan mode calls
   :cpp:`ResumeWeatherOverlaysAfterPan()`.

For dedicated page entry, first-enter behavior should be explicit and
idempotent. Existing providers use :cpp:`EnterPage()` and branch on
``first_enter``.

RASP and EDL reset behavior
---------------------------

RASP and EDL both preserve manual selections but reset auto-tracked
values when entering dedicated pages for the first time in a session.

- RASP:

  - :cpp:`WeatherUIState::ResetRaspForDedicatedPage()` clears time only
    when :cpp:`time_auto_advance` is true.
  - manual mode marks :cpp:`rasp.cursor_initialized = true`.

- EDL:

  - manual forecast/level marks
    :cpp:`edl.session.cursor_initialized = true`.
  - first dedicated-page entry without cursor initialization calls
    :cpp:`EDL::ResetForDedicatedPage()`.

SkySight forecast loading
-------------------------

SkySight keeps the selected forecast image visible while it refreshes the
catalog or fetches forecast-step metadata.  Selecting a forecast time queues
its download immediately; the dialog timer retries deferred metadata requests
without waiting for page activation or map rendering.

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

The SkySight cursor-bar rows are forecast time and layer.  Time selection is
session state.  Layer selection updates the current configured page and is
saved immediately, matching RASP layer selection.

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

Weather dialogs provide two placement actions:

- ``Add to page`` updates the currently configured page and replaces any
  existing weather overlay on that page.
- ``Add new page`` clones the currently configured page, forces map main,
  applies the selected overlay, and appends the clone as a new configured
  page.

Both actions:

- set ``bottom = WEATHER_CONTROLS`` so the shared weather cursor bar is active
- persist page layout changes to the profile immediately
- clear provider cursor initialization so first-enter behavior
  is reapplied consistently

When the page limit (``PageSettings::MAX_PAGES``) is reached, ``Add new page``
must fail without mutating existing pages, and the UI must show a clear
message.
