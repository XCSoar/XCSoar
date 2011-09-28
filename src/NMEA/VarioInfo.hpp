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

/** Derived vario data */
struct VarioInfo
{
  fixed sink_rate;

  /** Average vertical speed based on 30s */
  fixed average;
  /** Average vertical speed of the airmass based on 30s */
  fixed netto_average;

  /** Instant glide ratio */
  fixed ld;
  /** Glide ratio while in Cruise mode */
  fixed cruise_ld;

  /**
   * Average glide ratio.  Zero means the value is not available.
   */
  int average_ld;

  fixed ld_vario;

  /**
   * The lift of each ten degrees while circling.
   * Index 1 equals 5 to 15 degrees.
   */
  fixed lift_database[36];

  void ClearLiftDatabase();

  void Clear();
};

#endif
