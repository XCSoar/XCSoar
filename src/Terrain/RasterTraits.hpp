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

#ifndef XCSOAR_TERRAIN_RASTER_TRAITS_HPP
#define XCSOAR_TERRAIN_RASTER_TRAITS_HPP

/**
 * This namespace contains information on how the terrain raster works
 * in XCSoar.
 */
namespace RasterTraits {
  /**
   * The width and height of the terrain bitmap is shifted by this
   * number of bits to determine the overview size.
   */
  constexpr unsigned OVERVIEW_BITS = 4;

  constexpr unsigned OVERVIEW_MASK = (~0u) << OVERVIEW_BITS;

  /**
   * The fixed-point fractional part of sub-pixel coordinates.
   *
   * Do not edit!  There are still some hard-coded code sections left,
   * e.g. CombinedDivAndMod().
   */
  constexpr unsigned SUBPIXEL_BITS = 8;

  /**
   * Convert a pixel size to an overview pixel size, rounding down.
   */
  constexpr unsigned ToOverview(unsigned x) {
    return x >> OVERVIEW_BITS;
  }

  /**
   * Convert a pixel size to an overview pixel size, rounding up.
   */
  constexpr unsigned ToOverviewCeil(unsigned x) {
    return ToOverview(x + ~OVERVIEW_MASK);
  }
}

#endif
