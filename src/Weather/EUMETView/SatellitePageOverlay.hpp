// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Satellite.hpp"
#include "co/InvokeTask.hxx"
#include "net/AsyncTask.hpp"
#include "system/Path.hpp"
#include "time/BrokenDateTime.hpp"
#include "ui/event/Notify.hpp"
#include "ui/event/PeriodicTimer.hpp"

#include <exception>

class CurlGlobal;

/**
 * Fetches the satellite imagery tile by tile on the network
 * #EventLoop and installs each tile as a map overlay from the UI
 * thread.
 *
 * The imagery is drawn as a block of tiles rather than one image so
 * that a slow or intermittent link -- which is what a glider has --
 * still puts something useful on the map quickly.  The tiles are
 * requested nearest first, so the ground under the aircraft is drawn
 * within a few kilobytes and the picture then fills outwards; if the
 * connection dies half way, what is left is a complete picture of the
 * area that matters most.
 *
 * A tile already fetched is kept as the aircraft moves, so flying
 * across the block costs only the tiles that come into range.
 */
class SatelliteDownloadGlue final {
  CurlGlobal &curl;
  Net::AsyncTask task;
  UI::Notify complete_notify{[this]{ OnCompleteNotify(); }};

  /**
   * What the running download is for.  These belong to the glue
   * rather than to the caller, so that a request arriving while
   * another is in flight cannot relabel the image already on its way.
   */
  int layer_index = -1;
  GeoBitmap::TileData tile{};
  BrokenDateTime frame_time = BrokenDateTime::Invalid();

  AllocatedPath path{nullptr};
  std::exception_ptr completion_error;

  /**
   * Drives the sequence: picks up a newer frame, asks for the next
   * missing tile, and takes the picture down once it is too old.
   */
  UI::PeriodicTimer timer{[this]{ OnTimer(); }};

  Co::InvokeTask RunDownload();
  void OnCompletion(std::exception_ptr error) noexcept;
  void OnCompleteNotify() noexcept;
  void OnTimer() noexcept;

public:
  explicit SatelliteDownloadGlue(CurlGlobal &_curl) noexcept;

  ~SatelliteDownloadGlue() noexcept { BeginShutdown(); }

  SatelliteDownloadGlue(const SatelliteDownloadGlue &) = delete;
  SatelliteDownloadGlue &operator=(const SatelliteDownloadGlue &) = delete;

  [[nodiscard]]
  bool IsRunning() const noexcept { return task.IsRunning(); }

  void BeginShutdown() noexcept;

  /**
   * Fetch one tile, unless a download is already in flight.  Only one
   * request is ever outstanding: stacking them on a thin link makes
   * every tile arrive late instead of the nearest one arriving early.
   */
  void Start(int _layer_index, const GeoBitmap::TileData &_tile,
             const BrokenDateTime &_frame_time) noexcept;

  /** Begin driving the sequence. */
  void Schedule() noexcept;

  /** Stop, because no page shows the imagery any more. */
  void Cancel() noexcept;
};

/**
 * The shared #SatelliteDownloadGlue from #net_components (UI thread
 * only).
 */
SatelliteDownloadGlue *GetSatelliteDownloadGlue() noexcept;

namespace EUMETView {

/**
 * Show the imagery around the aircraft, fetching what is missing.
 */
void ActivatePageOverlay(int layer_index) noexcept;

/**
 * Take our tiles off the map, leaving any overlay somebody else put
 * there alone.
 */
void ClearMapOverlay() noexcept;

/**
 * Leave the satellite page: stop fetching and take the tiles down.
 */
void DeactivatePageOverlay() noexcept;

} // namespace EUMETView
