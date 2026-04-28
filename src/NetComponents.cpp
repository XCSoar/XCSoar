// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NetComponents.hpp"
#ifdef HAVE_TRACKING
#include "Tracking/TrackingGlue.hpp"
#endif
#ifdef HAVE_HTTP
#include "net/client/tim/Glue.hpp"
#include "NOTAM/NOTAMGlue.hpp"
#endif

NetComponents::NetComponents(EventLoop &event_loop, CurlGlobal &curl,
                             const TrackingSettings &tracking_settings,
                             const NOTAMSettings &notam_settings)
#ifdef HAVE_TRACKING
  :tracking(new TrackingGlue(event_loop, curl))
#endif
#ifdef HAVE_HTTP
# ifdef HAVE_TRACKING
  ,tim(new TIM::Glue(curl)),
   notam(new NOTAMGlue(notam_settings, curl))
# else
  :tim(new TIM::Glue(curl)),
   notam(new NOTAMGlue(notam_settings, curl))
# endif
#endif
{
#ifdef HAVE_TRACKING
  tracking->SetSettings(tracking_settings);
#else
  (void)tracking_settings;
  (void)event_loop;
#endif
#ifndef HAVE_HTTP
  (void)notam_settings;
#endif
#if !defined(HAVE_TRACKING) && !defined(HAVE_HTTP)
  (void)curl;
#endif
}

NetComponents::~NetComponents() noexcept = default;
