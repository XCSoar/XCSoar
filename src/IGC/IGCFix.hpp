/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#ifndef XCSOAR_IGC_FIX_HPP
#define XCSOAR_IGC_FIX_HPP

#include "Geo/GeoPoint.hpp"
#include "Time/BrokenTime.hpp"

struct NMEAInfo;

struct IGCFix
{
  BrokenTime time;

  GeoPoint location;

  bool gps_valid;

  int gps_altitude, pressure_altitude;

  /* extensions follow */

  /**
   * Engine noise level [0 to 999].  Negative if undefined.
   */
  int16_t enl;

  /**
   * Forward thrust, e.g. engine rpm [0 to 999].  Negative if
   * undefined.
   */
  int16_t rpm;

  /**
   * Magnetic heading [degrees].  Negative if undefined.
   */
  int16_t hdm;

  /**
   * True heading [degrees].  Negative if undefined.
   */
  int16_t hdt;

  /**
   * Magnetic track [degrees].  Negative if undefined.
   */
  int16_t trm;

  /**
   * True track [degrees].  Negative if undefined.
   */
  int16_t trt;

  /**
   * Ground speed [km/h].  Negative if undefined.
   */
  int16_t gsp;

  /**
   * Indicated airspeed [km/h].  Negative if undefined.
   */
  int16_t ias;

  /**
   * True airspeed [km/h].  Negative if undefined.
   */
  int16_t tas;

  /**
   * Satellites in use.  Negative if undefined.
   */
  int16_t siu;

  void ClearExtensions() {
    enl = rpm = -1;
    hdm = hdt = trm = trt = -1;
    gsp = ias = tas = -1;
    siu = -1;
  }

  void Clear() {
    time = BrokenTime::Invalid();
    ClearExtensions();
  }

  bool IsDefined() const {
    return time.IsPlausible();
  }

  /**
   * Copy data from the #NMEAInfo object into this.
   *
   * @return true if this object is a valid new fix
   */
  bool Apply(const NMEAInfo &basic);
};

#endif
