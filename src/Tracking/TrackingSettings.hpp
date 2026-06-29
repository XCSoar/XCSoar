// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Tracking/Features.hpp"

#ifdef HAVE_TRACKING

#include "Tracking/CloudSettings.hpp"
#include "Tracking/SkyLines/Features.hpp"
#include "Tracking/SkyLines/Settings.hpp"
#include "Tracking/LiveTrack24/Settings.hpp"

#include "util/StaticString.hxx"


struct TrackingSettings {
  SkyLinesTracking::Settings skylines;
  CloudSettings cloud;
  LiveTrack24::Settings livetrack24;

  void SetDefaults() {
    skylines.SetDefaults();
    cloud.SetDefaults();
    livetrack24.SetDefaults();
  }
};

#endif /* HAVE_TRACKING */
