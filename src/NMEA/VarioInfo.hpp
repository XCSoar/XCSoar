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

#ifndef XCSOAR_VARIO_INFO_HPP
#define XCSOAR_VARIO_INFO_HPP

#include "Math/fixed.hpp"

/**
 * Derived vario data
 * 
 */
struct VARIO_INFO
{
  fixed GliderSinkRate;

  /** GPS-based vario */
  fixed GPSVario;
  /** GPS-based vario including energy height */
  fixed GPSVarioTE;

  /**
   * Current vertical speed (total energy).  This is set to
   * TotalEnergyVario if available, and falls back to GPSVario.  It is
   * maintained by DeviceBlackboard::Vario().
   */
  fixed BruttoVario;

  /**
   * Vertical speed of air mass (m/s, up positive)
   */
  fixed NettoVario;

  /** Average vertical speed based on 30s */
  fixed Average30s;
  /** Average vertical speed of the airmass based on 30s */
  fixed NettoAverage30s;

  /** Instant glide ratio */
  fixed LD;
  /** Glide ratio while in Cruise mode */
  fixed CruiseLD;
  /** Average glide ratio */
  int AverageLD;

  fixed LDvario;

  /**
   * The lift of each ten degrees while circling.
   * Index 1 equals 5 to 15 degrees.
   */
  fixed LiftDatabase[36];

  void ClearLiftDatabase();

  void Clear();
};

#endif
