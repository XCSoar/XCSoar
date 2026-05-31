// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Tracking/Features.hpp"
#include "net/http/Features.hpp"
#include "Weather/Features.hpp"

#include <memory>

struct TrackingSettings;
struct NOTAMSettings;
class EventLoop;
class CurlGlobal;
class TrackingGlue;
namespace TIM { class Glue; }
class NOTAMGlue;
#ifdef HAVE_EDL
namespace EDL { class DownloadGlue; }
#endif

/**
 * Singleton for global networking-related objects.
 *
 * Background HTTP/coroutine work uses Net::AsyncTask and is stopped via
 * BeginShutdown() (also used for Net::DownloadManager and EDL tiles).
 * Blocking modal downloads use ShowCoDialog() on the UI thread.
 */
struct NetComponents {
#ifdef HAVE_TRACKING
  const std::unique_ptr<TrackingGlue> tracking;
#endif

#ifdef HAVE_HTTP
  const std::unique_ptr<TIM::Glue> tim;
  const std::unique_ptr<NOTAMGlue> notam;
# ifdef HAVE_EDL
  const std::unique_ptr<EDL::DownloadGlue> edl;
# endif
#endif

  NetComponents(EventLoop &event_loop, CurlGlobal &curl,
                const TrackingSettings &tracking_settings,
                const NOTAMSettings &notam_settings);
  ~NetComponents() noexcept;

  /** Cancel downloads and background network tasks before UI teardown. */
  void BeginShutdown() noexcept;

  NetComponents(const NetComponents &) = delete;
  NetComponents &operator=(const NetComponents &) = delete;
};
