/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#ifndef XCSOAR_NMEA_THERMAL_BAND_H
#define XCSOAR_NMEA_THERMAL_BAND_H

#include "Math/fixed.hpp"

/**
 * Derived thermal climb rate histogram by altitude (time averaged)
 * 
 */
struct ThermalBandInfo
{
  static const unsigned NUMTHERMALBUCKETS = 10;

  /** Height above working band/safety (m) */
  fixed working_band_height;

  /** Absolute ceiling of climbs (m) */
  fixed working_band_ceiling;

  /** Fraction of working band height */
  fixed working_band_fraction;

  /** Maximum height achieved in circling */ 
  fixed MaxThermalHeight;
  /** Number of samples in each bucket */ 
  int    ThermalProfileN[NUMTHERMALBUCKETS];
  /** Average climb rate in each bucket */ 
  fixed ThermalProfileW[NUMTHERMALBUCKETS];

  void Clear();

  /**
   * Calculates the bucket number for the specified height.
   */
  unsigned BucketForHeight(fixed height) const;

  /**
   * Calculates the base height of the specified bucket.
   */
  fixed BucketHeight(unsigned bucket) const;

  void Add(const fixed height, const fixed total_energy_vario);

private:
  void Expand(const fixed height);
};

#endif
