// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/GeoBounds.hpp"
#include "co/InvokeTask.hxx"
#include "net/AsyncTask.hpp"
#include "system/Path.hpp"
#include "ui/event/Notify.hpp"

#include <exception>

class CurlGlobal;

/**
 * Runs OPERA::DownloadRadar() on the network #EventLoop and installs
 * the finished image as the map overlay from the UI thread.
 *
 * Unlike the Weather dialog, which downloads inside a modal progress
 * dialog, a page overlay has to appear without the pilot waiting for
 * it, so the fetch runs in the background and the map gains the image
 * whenever it is ready.
 */
class RadarDownloadGlue final {
  CurlGlobal &curl;
  Net::AsyncTask task;
  UI::Notify complete_notify{[this]{ OnCompleteNotify(); }};

  /** the area the running download was requested for */
  GeoBounds bounds = GeoBounds::Invalid();
  unsigned width = 0, height = 0;

  AllocatedPath path{nullptr};
  std::exception_ptr completion_error;

  Co::InvokeTask RunDownload();
  void OnCompletion(std::exception_ptr error) noexcept;
  void OnCompleteNotify() noexcept;

public:
  explicit RadarDownloadGlue(CurlGlobal &_curl) noexcept;

  ~RadarDownloadGlue() noexcept { BeginShutdown(); }

  RadarDownloadGlue(const RadarDownloadGlue &) = delete;
  RadarDownloadGlue &operator=(const RadarDownloadGlue &) = delete;

  [[nodiscard]]
  bool IsRunning() const noexcept { return task.IsRunning(); }

  void BeginShutdown() noexcept;

  /**
   * Fetch the composite for the given area, unless one is already in
   * flight.
   */
  void Start(const GeoBounds &_bounds,
             unsigned _width, unsigned _height) noexcept;
};

/**
 * The shared #RadarDownloadGlue from #net_components (UI thread only).
 */
RadarDownloadGlue *GetRadarDownloadGlue() noexcept;

namespace OPERA {

/**
 * Request the composite for the area the map currently shows, and
 * install it when it arrives.
 */
void ActivatePageOverlay() noexcept;

/**
 * Remove the radar overlay, but only if the map is still showing the
 * one we installed.
 */
void ClearMapOverlay() noexcept;

} // namespace OPERA
