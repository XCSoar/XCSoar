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

#ifndef XCSOAR_WAVE_RESULT_HPP
#define XCSOAR_WAVE_RESULT_HPP

#include "Math/Angle.hpp"
#include "Geo/GeoPoint.hpp"
#include "Util/TrivialArray.hxx"

struct WaveInfo {
  GeoPoint location;

  GeoPoint a, b;
  Angle normal;

  /**
   * The time (see NMEAInfo::time) when this wave was calculated.
   * This is used to decay old waves.
   *
   * A negative value means a wallclock was not available at the time
   * this wave was calculated.
   */
  double time;

  static WaveInfo Undefined() {
    WaveInfo result;
    result.location = GeoPoint::Invalid();
    return result;
  }

  bool IsDefined() const {
    return location.IsValid();
  }

  double GetLength() const {
    return a.DistanceS(b);
  }
};

struct WaveResult {
  TrivialArray<WaveInfo, 32> waves;

  void Clear() {
    waves.clear();
  }
};

#endif
