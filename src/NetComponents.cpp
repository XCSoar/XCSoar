// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NetComponents.hpp"
#include "Tracking/TrackingGlue.hpp"
#include "net/client/tim/Glue.hpp"
#include "Traffic/Aggregator.hpp"

NetComponents::NetComponents(EventLoop &event_loop, CurlGlobal &curl,
                             const TrackingSettings &tracking_settings) noexcept
  :tracking(new TrackingGlue(event_loop, curl)),
   tim(new TIM::Glue(curl)),
   traffic_aggregator(new TrafficAggregator())
{
  tracking->SetSettings(tracking_settings);
}

NetComponents::~NetComponents() noexcept = default;
