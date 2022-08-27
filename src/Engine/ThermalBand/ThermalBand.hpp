/* Copyright_License {

   XCSoar Glide Computer - http://www.xcsoar.org/
   Copyright (C) 2000-2022 The XCSoar Project
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

#pragma once

#include "ThermalSlice.hpp"
#include "util/TrivialArray.hxx"

#include <type_traits>

/*
   Container of a stack of slices representing samples vertically
   through the atmosphere.  Spacing is even.  In order for statistics
   to be valid, there must have been passage through at least 2
   slices.  This class is not intended to be used directly, but via
   ThermalEncounterBand (for a single thermal climb encounter), and
   ThermalEncounterCollection (to collect statistics across several climbs).
 */
class ThermalBand {
  // maximum number of slices in the collection
  static constexpr unsigned NUM_SLICES = 64;

  // minimum time of activity for this band to be considered valid
  static constexpr FloatDuration MIN_VALID_TIME = std::chrono::seconds{30};

protected:
  double h_min;
  double dh;

  TrivialArray<ThermalSlice, NUM_SLICES> slices;

public:
  // height of base of this container
  constexpr double GetFloor() const noexcept {
    return h_min;
  }

  // height of ceiling of this container (including fractional height to top slice)
  constexpr double GetCeiling() const noexcept {
    return GetSliceHeight(slices.size());
  }

  // height of ceiling of this container (including fractional height in top slice)
  [[gnu::pure]]
  double GetSliceCenter(unsigned index) const noexcept;

  // height of slice at index
  constexpr double GetSliceHeight(const unsigned index) const noexcept {
    return h_min + index * dh;
  }

  constexpr double GetSpacing() const noexcept {
    return dh;
  }

  // are statistics valid for use?
  constexpr bool Valid() const noexcept {
    return size() > 1 && slices[size() - 1].time > MIN_VALID_TIME;
  }

  void Reset() noexcept;

  static constexpr std::size_t max_size() noexcept {
    return NUM_SLICES;
  }

  // number of slices in container
  constexpr unsigned size() const noexcept {
    return slices.size();
  }

  constexpr unsigned empty() const noexcept {
    return slices.empty();
  }

  // calculate the maximum number of thermal encounters across all elements
  [[gnu::pure]]
  double GetMaxN() const noexcept;
  // calculate the total time elapsed in climb across all elements
  [[gnu::pure]]
  FloatDuration GetTimeElapsed() const noexcept;
  // calculate the maximum climb rate across all elements
  [[gnu::pure]]
  double GetMaxW() const noexcept;

  // retrieve slice at index
  constexpr const ThermalSlice &GetSlice(const unsigned index) const noexcept {
    return slices[index];
  }

  // returns true if occupancy (n) is non-zero
  constexpr bool Occupied(const unsigned index) const noexcept {
    if (index >= size())
      return false;
    return slices[index].Occupied();
  }

protected:
  constexpr unsigned GetSliceIndex(const double height) const noexcept {
    return (height - h_min) / dh;
  }

  void Update(const unsigned index) noexcept;

  void Decimate(bool update) noexcept;
  void CheckExpand(const ThermalBand &tb, bool update) noexcept;
};

static_assert(std::is_trivial<ThermalBand>::value, "type is not trivial");
