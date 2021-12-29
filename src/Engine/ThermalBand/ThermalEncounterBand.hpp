/* Copyright_License {

   XCSoar Glide Computer - http://www.xcsoar.org/
   Copyright (C) 2000-2021 The XCSoar Project
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

#ifndef THERMAL_ENCOUNTER_BAND_HPP
#define THERMAL_ENCOUNTER_BAND_HPP

#include "ThermalBand.hpp"
#include "time/Stamp.hpp"

#include <type_traits>

class ThermalEncounterBand : public ThermalBand
{
  TimeStamp time_start;

public:
  void Reset() noexcept {
    ThermalBand::Reset();
    time_start = TimeStamp::Undefined();
  }

  void AddSample(const TimeStamp time,
                 const double height) noexcept;

private:
  unsigned ResizeToHeight(const double height);

  unsigned FindPenultimateFinished(const unsigned index,
                                   const FloatDuration time) noexcept;

  FloatDuration EstimateTimeStep(const FloatDuration time,
                                 const double height,
                                 const unsigned index) noexcept;

  void Start(const TimeStamp time,
             const double height);

};

static_assert(std::is_trivial<ThermalEncounterBand>::value, "type is not trivial");

#endif
