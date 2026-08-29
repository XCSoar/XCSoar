// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Radar.hpp"
#include "co/InvokeTask.hxx"
#include "net/AsyncTask.hpp"
#include "system/Path.hpp"
#include "time/BrokenDateTime.hpp"
#include "ui/event/Notify.hpp"
#include "ui/event/PeriodicTimer.hpp"

#include <exception>
#include <string>

class CurlGlobal;

/**
 * Fills the map with the radar composite, one display tile at a time,
 * from underneath the aircraft outwards.
 *
 * The composite is a single five megabyte file covering the continent,
 * so it is read with range requests: one for its directory, then one
 * per tile of it that the block actually needs.  Everything runs on
 * the network #EventLoop; finished tiles are installed as map
 * overlays from the UI thread.
 *
 * Unlike the Weather dialog, which downloads inside a modal progress
 * dialog, a page overlay has to appear without the pilot waiting for
 * it.
 */
class RadarDownloadGlue final {
  CurlGlobal &curl;
  Net::AsyncTask task;
  UI::Notify complete_notify{[this]{ OnCompleteNotify(); }};

  /**
   * The frame being read, and the tiles of it decoded so far.  Held
   * across display tiles, so that a composite is opened once and each
   * of its tiles travels once however many display tiles it feeds.
   *
   * Network thread only.
   */
  OPERA::RadarComposite composite;

  /* what the running download is for; written by the UI thread before
     the task starts, read by the network thread while it runs */
  std::string url;

  /**
   * The tile the block is centred on, which is what the pyramid level
   * is chosen from.
   *
   * One level for the whole block, rather than one per tile: the
   * block spans enough latitude at a wide zoom that a per-tile choice
   * lands on two different levels, and then the same ground travels
   * twice and meets itself as a resolution seam across the map.
   */
  GeoBitmap::TileData base_tile{};

  GeoBitmap::TileData tile{};
  BrokenDateTime frame_time = BrokenDateTime::Invalid();
  unsigned slot = 0;

  AllocatedPath path{nullptr};

  /** did the finished tile hold any echo worth installing? */
  bool drawn = false;

  std::exception_ptr completion_error;

  /**
   * Watches the age of the frame on the map while a radar page is
   * open: it fetches a newer one as soon as one exists, and takes the
   * picture down once it is older than OPERA::MAX_AGE_MINUTES.
   */
  UI::PeriodicTimer age_timer{[this]{ OnAgeTimer(); }};

  Co::InvokeTask RunDownload();
  void OnCompletion(std::exception_ptr error) noexcept;
  void OnCompleteNotify() noexcept;
  void OnAgeTimer() noexcept;

public:
  explicit RadarDownloadGlue(CurlGlobal &_curl) noexcept;

  ~RadarDownloadGlue() noexcept { BeginShutdown(); }

  RadarDownloadGlue(const RadarDownloadGlue &) = delete;
  RadarDownloadGlue &operator=(const RadarDownloadGlue &) = delete;

  [[nodiscard]]
  bool IsRunning() const noexcept { return task.IsRunning(); }

  void BeginShutdown() noexcept;

  /**
   * Fetch one display tile, unless a download is already in flight.
   */
  void Start(std::string _url, const GeoBitmap::TileData &_base,
             const GeoBitmap::TileData &_tile,
             const BrokenDateTime &_frame_time, unsigned _slot) noexcept;

  /** Begin watching the age of the displayed frame. */
  void ScheduleAgeCheck() noexcept;

  /** Stop watching, because no page shows the radar any more. */
  void CancelAgeCheck() noexcept;
};

/**
 * The shared #RadarDownloadGlue from #net_components (UI thread only).
 */
RadarDownloadGlue *GetRadarDownloadGlue() noexcept;

namespace OPERA {

/**
 * Fetch and draw the block around the aircraft, and keep it filling.
 *
 * Safe to call as often as the map changes: it starts a download only
 * when there is a tile missing and nothing already in flight.
 */
void ActivatePageOverlay() noexcept;

/**
 * Take the radar off the map, leaving alone any overlay somebody else
 * installed.
 */
void ClearMapOverlay() noexcept;

/**
 * Leave the radar page: stop watching the frame's age and take it
 * off the map.
 *
 * Does nothing while the overlay is suspended for a pan.
 */
void DeactivatePageOverlay() noexcept;

/**
 * Hold the radar on the map across a pan.
 *
 * Panning shows the fullscreen map, whose layout carries no overlay,
 * so the page machinery deactivates the radar on the way in and would
 * fetch the whole block again on the way out.  Between these two
 * calls #DeactivatePageOverlay() is ignored and the tiles stay where
 * they are.
 */
void SuspendForPan() noexcept;

/** Let the radar be taken off the map again. */
void ResumeAfterPan() noexcept;

} // namespace OPERA
