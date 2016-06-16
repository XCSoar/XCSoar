/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef XCSOAR_TRACKING_SKYLINES_HANDLER_HPP
#define XCSOAR_TRACKING_SKYLINES_HANDLER_HPP

#include "Features.hpp"

#ifdef HAVE_SKYLINES_TRACKING_HANDLER

#include <stdint.h>
#include <tchar.h>

struct GeoPoint;
struct AGeoPoint;

namespace SkyLinesTracking {

class Handler {
public:
  virtual void OnAck(unsigned id) {}
  virtual void OnTraffic(uint32_t pilot_id, unsigned time_of_day_ms,
                         const ::GeoPoint &location, int altitude) {}
  virtual void OnUserName(uint32_t user_id, const TCHAR *name) {}
  virtual void OnWave(unsigned time_of_day_ms,
                      const ::GeoPoint &a, const ::GeoPoint &b) {}
  virtual void OnThermal(unsigned time_of_day_ms,
                         const AGeoPoint &bottom, const AGeoPoint &top,
                         double lift) {}
};

} /* namespace SkyLinesTracking */

#endif

#endif
