// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Tracking/Features.hpp"
#include "net/http/Features.hpp"

#include <memory>

struct TrackingSettings;
class EventLoop;
class CurlGlobal;
class TrackingGlue;
namespace TIM { class Glue; }

/**
 * This singleton manages global networking-related objects.
 */
struct NetComponents {
#ifdef HAVE_TRACKING
  const std::unique_ptr<TrackingGlue> tracking;
#endif

#ifdef HAVE_HTTP
  const std::unique_ptr<TIM::Glue> tim;
#endif

  NetComponents(EventLoop &event_loop, CurlGlobal &curl,
                const TrackingSettings &tracking_settings) noexcept;
  ~NetComponents() noexcept;

  NetComponents(const NetComponents &) = delete;
  NetComponents &operator=(const NetComponents &) = delete;
};
