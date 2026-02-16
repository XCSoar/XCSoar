// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Tracking/Features.hpp"

#ifdef HAVE_TRACKING

#include "Tracking/SkyLines/Handler.hpp"
#include "Tracking/SkyLines/Glue.hpp"
#include "Tracking/SkyLines/Data.hpp"
#include "Tracking/LiveTrack24/Glue.hpp"

struct TrackingSettings;
struct MoreData;
struct DerivedInfo;
class CurlGlobal;

class TrackingGlue final
  : private SkyLinesTracking::Handler
{
  SkyLinesTracking::Glue skylines;

  SkyLinesTracking::Data skylines_data;

  LiveTrack24::Glue livetrack24;

public:
  TrackingGlue(EventLoop &event_loop, CurlGlobal &curl) noexcept;

  void SetSettings(const TrackingSettings &_settings);

  void OnTimer(const MoreData &basic, const DerivedInfo &calculated);

private:
  /* virtual methods from SkyLinesTracking::Handler */
  virtual void OnTraffic(uint32_t pilot_id, unsigned time_of_day_ms,
                         const GeoPoint &location, int altitude) override;
    virtual void OnUserName(uint32_t user_id, const char *name) override;
  void OnWave(unsigned time_of_day_ms,
              const GeoPoint &a, const GeoPoint &b) override;
  void OnThermal(unsigned time_of_day_ms,
                 const AGeoPoint &bottom, const AGeoPoint &top,
                 double lift) override;
  void OnSkyLinesError(std::exception_ptr e) override;

public:
  const SkyLinesTracking::Data &GetSkyLinesData() const {
    return skylines_data;
  }
};

#endif /* HAVE_TRACKING */
