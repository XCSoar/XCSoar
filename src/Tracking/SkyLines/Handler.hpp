// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Features.hpp"

#include <exception>

#include <cstdint>
#include <tchar.h>

struct GeoPoint;
struct AGeoPoint;

namespace SkyLinesTracking {

class Handler {
public:
  /**
   * Called as soon as the UDP socket has been created and is
   * available for I/O.  This does not imply that the SkyLines
   * server is really available.
   */
  virtual void OnSkyLinesReady() {}

  virtual void OnAck([[maybe_unused]] unsigned id) {}
  virtual void OnTraffic([[maybe_unused]] uint32_t pilot_id, [[maybe_unused]] unsigned time_of_day_ms,
                         [[maybe_unused]] const ::GeoPoint &location, [[maybe_unused]] int altitude) {}
  virtual void OnUserName([[maybe_unused]] uint32_t user_id, [[maybe_unused]] const char *name) {}
  virtual void OnWave([[maybe_unused]] unsigned time_of_day_ms,
                      [[maybe_unused]] const ::GeoPoint &a, [[maybe_unused]] const ::GeoPoint &b) {}
  virtual void OnThermal([[maybe_unused]] unsigned time_of_day_ms,
                         [[maybe_unused]] const AGeoPoint &bottom, [[maybe_unused]] const AGeoPoint &top,
                         [[maybe_unused]] double lift) {}

  /**
   * An error has occurred, and the SkyLines tracking client is
   * defunct.  To make restore its function, call Client::Open()
   * again.
   */
  virtual void OnSkyLinesError(std::exception_ptr e) = 0;
};

} /* namespace SkyLinesTracking */
